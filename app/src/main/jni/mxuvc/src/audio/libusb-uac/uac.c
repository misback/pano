/*******************************************************************************
* 
* The content of this file or document is CONFIDENTIAL and PROPRIETARY
* to GEO Semiconductor.  It is subject to the terms of a License Agreement 
* between Licensee and GEO Semiconductor, restricting among other things,
* the use, reproduction, distribution and transfer.  Each of the embodiments,
* including this information and any derivative work shall retain this 
* copyright notice.
* 
* Copyright 2013-2016 GEO Semiconductor, Inc.
* All rights reserved.
*
* 
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>
#include <inttypes.h>

#include <assert.h>

#include "common.h"
#include "mxuvc.h"
#include "adesc-parser.h"
#include "libusb/handle_events.h"

#define USB_DIR_OUT                     0               /* to device */
#define USB_DIR_IN                      0x80            /* to host */

/*
* USB types, the second of three bRequestType fields
*/
#define USB_TYPE_MASK                   (0x03 << 5)
#define USB_TYPE_STANDARD               (0x00 << 5)
#define USB_TYPE_CLASS                  (0x01 << 5)
#define USB_TYPE_VENDOR                 (0x02 << 5)
#define USB_TYPE_RESERVED               (0x03 << 5)

/*
* USB recipients, the third of three bRequestType fields
*/
#define USB_RECIP_MASK                  0x1f
#define USB_RECIP_DEVICE                0x00
#define USB_RECIP_INTERFACE             0x01
#define USB_RECIP_ENDPOINT              0x02
#define USB_RECIP_OTHER                 0x03

/******************
 *    UAC defs    *
 *****************/
#define AC_MUTE_CONTROL         0x01
#define AC_VOLUME_CONTROL       0x02

/* Probe requests */
#define SET_CUR                 0x01
#define GET_CUR                 0x81
#define GET_MIN                 0x82
#define GET_MAX                 0x83
#define GET_RES                 0x84
#define GET_LEN                 0x85
#define GET_INFO                0x86
#define GET_DEF                 0x87

/* Invoke the user callback every AUDIO_DURATION_MS ms worth of data*/
#define AUDIO_DURATION_MS_DEFAULT       10
/* Audio USB isoc frames to request */
#define PACKETS_PER_TRANSFER_DEFAULT    5
#define NUM_TRANSFERS_DEFAULT           50
#define AUDIO_SAMPLING_RATE_DEFAULT     24000

#define AUD_BIT_PER_SAMPLE      16
#define AUD_NUM_CH              2

/* Control request type */
#define REQ_SET         0x21
#define REQ_GET         0xa1

/**************************
 *    Global variables    *
 *************************/
extern struct audio_cfg aud_cfg;

static mxuvc_audio_cb_t aud_cb;
static audio_format_t cur_aud_format;
static void *aud_cb_user_data;
static unsigned int aud_frame_count = 0;
static unsigned int aud_target_size = 0;
static int aud_started;
static int aud_first_transfer;
static int audio_dead;
static volatile int active_transfers = 0;
static unsigned char *abuf; /* Audio buffer, filled by libusb */

//static int exit_audio_event_loop;
//static pthread_t audio_event_thread;
static struct libusb_device_handle *audio_hdl;
static struct libusb_context       *audio_ctx;
static volatile int audio_disconnected = 1;
static volatile int audio_initialized = 0;

static unsigned int packets_per_transfer;
static unsigned int audio_duration_ms;
static unsigned int num_transfers;

/* TODO: number of supported channels to be filled by parsing the descriptor */
static unsigned int audio_num_channels = 1;

/*******************
 *    Functions    *
 *******************/
struct audio_packet {
	unsigned int size;
	int format;
	unsigned char *buffer;
};

typedef enum ctrl_id {
	CTRL_NONE   = 0,
	CTRL_MUTE   = 1,
	CTRL_VOLUME = 2,
} ctrl_id_t;

typedef struct {
	ctrl_id_t id;
	int unit;
	int cs;
	int cn;
	int len;
	int16_t min;
	int16_t max;
	uint16_t res;
} AUDIO_CTRL;

#define FEATURE 0
/* Fill min, max ans res with random values.
 * They will will overwritten during audio_init() */
static AUDIO_CTRL uac_controls[] = {
	{CTRL_MUTE,    FEATURE,     AC_MUTE_CONTROL,    0, 1, 0, 1, 1},
	{CTRL_VOLUME,  FEATURE,     AC_VOLUME_CONTROL,  1, 2, 0, 1, 1},
	{CTRL_NONE, 0, 0, 0, 0, 0, 0, 0}
};

static inline void incr_active_transfers() {
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&mutex);
	active_transfers++;
	pthread_mutex_unlock(&mutex);
}
static inline void decr_active_transfers() {
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&mutex);
	active_transfers--;
	pthread_mutex_unlock(&mutex);
}
static inline int get_active_transfers() {
	int val;
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&mutex);
	val = active_transfers;
	pthread_mutex_unlock(&mutex);
	return val;
}
static inline int set_active_transfers(int val) {
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&mutex);
	active_transfers = val;
	pthread_mutex_unlock(&mutex);
	return val;
}

static AUDIO_CTRL* get_ctrl_by_id(ctrl_id_t id)
{
	AUDIO_CTRL *control = uac_controls;
	while(control->id != CTRL_NONE) {
		if(control->id == id)
			return control;
		control++;
	}

	ERROR(NULL, "Unexpected error: no match for ctrl_id %i in "
			"get_ctrl_by_id()\n", (int)id);
}
static int get_ctrl(ctrl_id_t id, int get_type, void *val)
{
	int ret;
	AUDIO_CTRL *ctrl = NULL;
	uint8_t bmRequestType = REQ_GET;
	uint8_t bRequest = get_type;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	unsigned char *data = (unsigned char *) val;

	ctrl  = get_ctrl_by_id(id);
	if(ctrl == NULL)
		return -1;

	wValue = (ctrl->cs << 8) + ctrl->cn;
	wLength = ctrl->len;
	wIndex = aud_cfg.ctrlif + (ctrl->unit << 8);

	ret = libusb_control_transfer(audio_hdl,
			bmRequestType,
			bRequest,
			wValue,
			wIndex,
			data,
			wLength,
			USB_TIMEOUT);

	return ret;
}

static int set_ctrl(ctrl_id_t id, int set_type, void *val)
{
	int ret;
	AUDIO_CTRL *ctrl;
	uint8_t bmRequestType = REQ_SET;
	uint8_t bRequest = set_type;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	unsigned char *data = (unsigned char *) val;

	ctrl = get_ctrl_by_id(id);
	if(ctrl == NULL)
		return -1;

	wValue = (ctrl->cs << 8) + ctrl->cn;
	wLength = ctrl->len;
	wIndex = aud_cfg.ctrlif + (ctrl->unit << 8);

	ret = libusb_control_transfer(audio_hdl,
			bmRequestType,
			bRequest,
			wValue,
			wIndex,
			data,
			wLength,
			USB_TIMEOUT);

	return ret;
}

static void audio_removed(int fd, void *user_data)
{
	TRACE("Audio removed notification.\n");
	audio_disconnected = 1;
}

/* Initialize the audio path */
int mxuvc_audio_init(const char *backend, const char *options)
{
	RECORD("\"%s\", \"%s\"", backend, options);
	struct libusb_device *dev = NULL;
	int ret=0, i, config;
	uint16_t vendor_id=0xdead, product_id=0xbeef;
	char *str=NULL, *opt, *value;
	int audio_sampling_rate;

	TRACE("Initializing the audio\n");

	/* Check that the correct video backend was requested*/
	if(strncmp(backend, "libusb-uac", 10)) {
		ERROR(-1, "The audio backend requested (%s) does not match "
			"the implemented one (libusb-uac)", backend);
	}

	/* Set init parameters to their default values */
	packets_per_transfer = PACKETS_PER_TRANSFER_DEFAULT;
	num_transfers        = NUM_TRANSFERS_DEFAULT;
	audio_duration_ms    = AUDIO_DURATION_MS_DEFAULT;
	audio_sampling_rate  = AUDIO_SAMPLING_RATE_DEFAULT;

	/* Copy the options string to a new buffer since next_opt() needs
	 * non const strings and options could be a const string */
	if(options != NULL) {
		str = (char*)malloc(strlen(options)+1);
		strncpy(str, options, strlen(options));
		*(str + strlen(options)) = '\0';
	}

	/* Get backend option from the option string */
	ret = next_opt(str, &opt, &value);
	while(ret == 0) {
		if(strncmp(opt, "vid", 3) == 0) {
			vendor_id = (uint16_t) strtoul(value, NULL, 16);
		} else if(strncmp(opt, "pid", 3) == 0) {
			product_id = (uint16_t) strtoul(value, NULL, 16);
		} else if(strncmp(opt, "packets_per_transfer", 19) == 0) {
			packets_per_transfer = (unsigned int) strtoul(value, NULL, 10);
		} else if(strncmp(opt, "num_transfers", 12) == 0) {
			num_transfers = (unsigned int) strtoul(value, NULL, 10);
		} else if(strncmp(opt, "audio_duration_ms", 17) == 0) {
			audio_duration_ms = (unsigned int) strtoul(value, NULL, 10);
		}
		else if (strncmp (opt, "audio_sampling_rate", 19) == 0) {
			audio_sampling_rate =
				(unsigned int) strtoul (value, NULL, 10);
		} else {
			WARNING("Unrecognized option: '%s'", opt);
		}
		ret = next_opt(NULL, &opt, &value);
	}

	/* Display the values we are going to use */
	TRACE("Using vid = 0x%x\n",                vendor_id);
	TRACE("Using pid = 0x%x\n",                product_id);
	TRACE("Using packets_per_transfer = %i\n", packets_per_transfer);
	TRACE("Using num_transfers = %i\n",        num_transfers);
	TRACE("Using audio_duration_ms = %i\n",    audio_duration_ms);
	TRACE("Using audio_sampling_rate = %i\n",  audio_sampling_rate);

	/* Free the memory allocated to parse 'options' */
	if(str)
		free(str);

	/* Initialize the backend */
	aud_started = 0;
	audio_disconnected = 0;
	ret = init_libusb(&audio_ctx);
	if(ret < 0)
		return -1;

	audio_hdl = libusb_open_device_with_vid_pid(audio_ctx, vendor_id,
							product_id);
	CHECK_ERROR(audio_hdl == NULL, -1, "Could not open USB device "
			"%x:%x", vendor_id, product_id);

	dev = libusb_get_device(audio_hdl);
	if(dev == NULL) {
		printf("Unexpected error: libusb_get_device returned a NULL "
				"pointer.");
		mxuvc_audio_deinit();
		return -1;
	}

	/* Get active USB configuration */
	libusb_get_configuration(audio_hdl, &config);

	/* Parse USB decriptors from active USB configuration
	 * to get all the UVC/UAC info needed */
	ret = aparse_usb_config(dev, config);
	if(ret < 0){
		mxuvc_audio_deinit();
		return -1;
	}

	/* Initialize audio */

	/* Claim audio control interface */
	/* Check if a kernel driver is active on the audio control interface */
	ret = libusb_kernel_driver_active(audio_hdl, aud_cfg.ctrlif);
	if(ret < 0)
		printf("Error: libusb_kernel_driver_active failed %d\n", ret);

	if(ret == 1) {
		TRACE("Detach the kernel driver...\n");
		/* If kernel driver is active, detach it so that we can claim
		 * the interface */
		ret = libusb_detach_kernel_driver(audio_hdl, aud_cfg.ctrlif);
		if(ret < 0)
			printf("Error: libusb_detach_kernel_driver failed "
					"%d\n", ret);
	}

	/* Claim audio control interface */
	ret = libusb_claim_interface(audio_hdl, aud_cfg.ctrlif);
	if(ret < 0) {
		printf("Error: libusb_claim_interface failed %d\n", ret);
	}

	/* Claim audio streaming interface */
	/* Check if a kernel driver is active on the audio interface */
	ret = libusb_kernel_driver_active(audio_hdl, aud_cfg.interface);
	if(ret < 0)
		printf("Error: libusb_kernel_driver_active failed %d\n", ret);

	if(ret == 1) {
		TRACE("Detach the kernel driver...\n");
		/* If kernel driver is active, detach it so that we can claim
		 * the interface */
		ret = libusb_detach_kernel_driver(audio_hdl, aud_cfg.interface);
		if(ret < 0)
			printf("Error: libusb_detach_kernel_driver failed "
					"%d\n",ret);
	}

	/* Claim audio streaming interface */
	ret = libusb_claim_interface(audio_hdl, aud_cfg.interface);
	if(ret < 0) {
		printf("Error: libusb_claim_interface failed %d\n",ret);
	}

	/* Select sampling rate */
	for(i=0;i<MAX_AUD_FMTS;i++) {
		if(aud_cfg.format[i].samFr == audio_sampling_rate){
			aud_cfg.fmt_idx = i;
			break;
		}
		CHECK_ERROR(i == MAX_AUD_FMTS-1, -1,
			"Unable to set the sampling rate to %i",
			audio_sampling_rate);
	}

	/* Map default UAC format to Audio format */
	cur_aud_format = AUD_FORMAT_PCM_RAW;

	/* Get min, max and real unit id for ctrl */
	AUDIO_CTRL *ctrl = uac_controls;
	int16_t min = 0, max = 0;
	uint16_t res = 0;
	while(ctrl->id != CTRL_NONE) {
		switch(ctrl->unit) {
			TRACE(">>>>>id:%d  unit:%d\n", ctrl->id,ctrl->unit);
			case FEATURE:
				ctrl->unit = aud_cfg.ctrl_feature;
				break;
			default:
				ERROR(-1, "Unsupported control unit (%i) for "
						"audio control %i",
						ctrl->unit, ctrl->id);
		}

		if (ctrl->id == CTRL_MUTE) {
			ctrl++;
			continue;
		}

		ret = get_ctrl(ctrl->id, GET_MIN, (void*) &min);
		CHECK_ERROR(ret < 0, -1,
				"Unable to get min (GET_MIN) for audio "
				"control: id=%i, cs=%i, cn=%i.",
				ctrl->id, ctrl->cs, ctrl->cn);
		ctrl->min = min;
		ret = get_ctrl(ctrl->id, GET_MAX, (void*) &max);
		CHECK_ERROR(ret < 0, -1,
				"Unable to get max (GET_MAX) for audio "
				"control: id=%i, cs=%i, cn=%i.",
				ctrl->id, ctrl->cs, ctrl->cn);
		ctrl->max = max;
		ret = get_ctrl(ctrl->id, GET_RES, (void*) &res);
		CHECK_ERROR(ret < 0, -1,
				"Unable to get res (GET_RES) for audio "
				"control: id=%i, cs=%i, cn=%i.",
				ctrl->id, ctrl->cs, ctrl->cn);
		ctrl->res = res;

		ctrl++;
	}

	/* Register removal USB event*/
	register_libusb_removal_cb((libusb_pollfd_removed_cb) audio_removed,
			audio_hdl);

	/* Start event thread/loop */
	ret = start_libusb_events();
	if(ret < 0)
		return -1;

	audio_initialized = 1;

	return 0;
}

/* Deinitialize the audio path */
int mxuvc_audio_deinit()
{
	RECORD("");
	int ret, i;
	int int_num[2] = {aud_cfg.ctrlif, aud_cfg.interface};

	if(!audio_initialized)
		return 1;

	deregister_libusb_removal_cb((libusb_pollfd_removed_cb) audio_removed);

	if(!audio_disconnected) {
		/* Stop the audio first if not stopped yet */
		if(aud_started == 1)
			mxuvc_audio_stop(AUD_CH1);

		for(i=0; i<2; i++) {
			TRACE2("Releasing audio interface %i\n", int_num[i]);
			ret = libusb_release_interface(audio_hdl, int_num[i]);
			CHECK_ERROR(ret < 0 && ret != LIBUSB_ERROR_NOT_FOUND &&
					ret != LIBUSB_ERROR_NO_DEVICE, -1,
					"Unable to release USB interface %i. Libusb "
					"return code is: %i", int_num[i], ret);
		}
		/* Re-attach the previously attached kernel driver */
		for(i=0; i<2; i++)
			libusb_attach_kernel_driver(audio_hdl, int_num[i]);
	}

	/* Exit Camera Event loop/thread */
	stop_libusb_events();

	/* Free USB resources */
	TRACE("Freeing USB audio resources\n");
	//if(mxuvc_audio_alive())
		libusb_close(audio_hdl);
	exit_libusb(&audio_ctx);
	if(aud_cfg.xfers) {
		free(aud_cfg.xfers);
		aud_cfg.xfers = NULL;
	}
	if(abuf) {
		free(abuf);
		abuf = NULL;
	}
	audio_hdl = NULL;
	audio_ctx = NULL;

	audio_initialized = 0;
	TRACE("The audio has been successfully uninitialized\n");

	//mxuvc_debug_stoprec();
	return 0;
}

int mxuvc_audio_alive()
{
	if (!audio_disconnected)
		return 1;
	else
		return 0;
}

static void free_audio_transfer(struct libusb_transfer **transfers,
		unsigned id)
{
	if(transfers == NULL || transfers[id] == NULL)
		return;

	libusb_free_transfer(transfers[id]);
	transfers[id] = NULL;
}

/* Callback function called upon arrival of ISOC audio data */
static void aud_cap_callback(struct libusb_transfer *transfer)
{
	static struct audio_packet packet;
	static int dead_counter = 0;
	static intptr_t prev_xfer_idx = -1;
	static uint32_t ts = 0;
	struct libusb_iso_packet_descriptor *descriptor;
	unsigned char* buffer;
	unsigned int i;
	intptr_t xfer_idx;

	assert(transfer != NULL);

	xfer_idx = (intptr_t) transfer->user_data;

	decr_active_transfers();

	/* Check if audio just started. Reset static values in that case */
	if(aud_first_transfer) {
		aud_first_transfer = 0;
		packet.size = 0;
		prev_xfer_idx = -1;
	}

	switch(transfer->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		break;
	case LIBUSB_TRANSFER_CANCELLED:
		free_audio_transfer(aud_cfg.xfers, xfer_idx);
		TRACE2("Audio transfer cancelled\n");
		return;
	case LIBUSB_TRANSFER_NO_DEVICE:
		free_audio_transfer(aud_cfg.xfers, xfer_idx);
		if(!audio_disconnected) {
			ERROR_NORET("The camera has been disconnected.");
			audio_disconnected = 1;
		}
		return;
	case LIBUSB_TRANSFER_ERROR:
		free_audio_transfer(aud_cfg.xfers, xfer_idx);
		if(!audio_disconnected) {
			ERROR_NORET("The audio USB transfer did not complete.");
			audio_disconnected = 1;
		}
		return;
	case LIBUSB_TRANSFER_TIMED_OUT:
		ERROR_NORET("The audio USB transfer timed-out.");
		if(aud_started && libusb_submit_transfer(transfer)>=0)
			incr_active_transfers();
		else
			free_audio_transfer(aud_cfg.xfers, xfer_idx);
		return;
	case LIBUSB_TRANSFER_STALL:
		ERROR_NORET("The audio USB transfer stalled.");
		if(aud_started && libusb_submit_transfer(transfer)>=0)
			incr_active_transfers();
		else
			free_audio_transfer(aud_cfg.xfers, xfer_idx);
		return;
	case LIBUSB_TRANSFER_OVERFLOW:
		ERROR_NORET("The audio USB transfer overflowed.");
		if(aud_started && libusb_submit_transfer(transfer)>=0)
			incr_active_transfers();
		else
			free_audio_transfer(aud_cfg.xfers, xfer_idx);
		return;
	default:
		ERROR_NORET("Unknown audio USB transfer return status.");
		if(aud_started && libusb_submit_transfer(transfer)>=0)
			incr_active_transfers();
		else
			free_audio_transfer(aud_cfg.xfers, xfer_idx);
		return;
	}

	/* Do nothing if the audio has been stopped */
	if(aud_started == 0) {
		free_audio_transfer(aud_cfg.xfers, xfer_idx);
		return;
	}

	if(prev_xfer_idx >= 0 && xfer_idx != (prev_xfer_idx + 1) % (int)num_transfers)
		WARNING("Audio transfer discontinuity: expected %" PRIiPTR
				", got %" PRIiPTR "\n",
				prev_xfer_idx + 1, xfer_idx);
	prev_xfer_idx = xfer_idx;

#if DEBUG_AUDIO_CALLBACK > 0
	/* Get time for further monitoring */
	struct timeval tv_start, tv_end;
	struct timezone tz;
	int delta_time;
	static int max_delta_time = 0, time_counter = 0;
	gettimeofday(&tv_start,&tz);
#endif

	/* Process audio packets */
	for(i=0; i<packets_per_transfer; i++) {
		descriptor = &transfer->iso_packet_desc[i];
		buffer = libusb_get_iso_packet_buffer_simple(transfer, i);

		if(descriptor->actual_length <= 0) {
			//if(packet.size > 0) {
			//	SHOW(packet.size);
			//	SHOW(aud_target_size);
			//}
			packet.size = 0;
			dead_counter++;
			if(dead_counter > 1000) { /* 1000 * 1ms = 1s */
				TRACE(  "####################################\n"
					"         Audio seems dead!!         \n"
					" No data has been received from the \n"
					"        camera for one second       \n"
					"####################################\n");
				dead_counter = 0;
				audio_dead = 1;
			}
			continue;
		}
		dead_counter = 0;

		/* "Append" data to current audio 'packet' */
		if(packet.size + descriptor->actual_length > aud_target_size) {
			WARNING("Received audio packet bigger (%i bytes) "
				"than targeted size (%i bytes). Skipping\n",
				packet.size + descriptor->actual_length,
				aud_target_size);
			packet.size = 0;
			continue;
		}
		if (packet.size == 0) {
			packet.buffer = buffer;
		}

		packet.size += descriptor->actual_length;
		aud_frame_count++;

		/* We have a full audio packet, let's process it */
		if (packet.size >= aud_target_size) {
#if DEBUG_AUDIO_QUEUE > 0
			if(packet.size != aud_target_size) {
				WARNING("Received audio packet is %i bytes "
					"what does not match targeted size "
					"(%i bytes).\nLast transfer size "
					"was %i.",
					packet.size, aud_target_size,
					descriptor->actual_length);
			}
#endif
			/* Handle buffer wrapping by memcpying the audio
			 * wrapped at the beginning at the end of the audio buffer
			 * ring, in the spare area allocated for that purpose
			 * during setup_isoc_transfer() */
			if (buffer < packet.buffer) {
				unsigned int packet_size = aud_cfg.format[aud_cfg.fmt_idx].pkt_size;
				/* Just in case: make sure we are not overflowing */
				if((unsigned int) (buffer - abuf + descriptor->actual_length) >
						10*packet_size) {
					SHOW(10*packet_size);
					SHOW((unsigned int) (buffer - abuf + descriptor->actual_length));
					TRACE("buffer = %p\n", buffer);
					TRACE("abuf = %p\n", abuf);
					SHOW(descriptor->actual_length);
					packet.buffer = buffer;
					packet.size = descriptor->actual_length;
					continue;
					//assert(buffer - abuf + descriptor->actual_length <= 10*packet_size);
				} else {
					memcpy(abuf + packets_per_transfer*packet_size*num_transfers,
						abuf, buffer - abuf + descriptor->actual_length);
				}
			}
			packet.format = cur_aud_format;

			if(aud_cb == NULL)
				WARNING("Audio callback function not registered.");
			else {
				/* Create a timestamp here */
				struct timeval tv;
				uint64_t ts64 = 0, tsec = 0, tusec = 0;
				audio_params_t param;

				gettimeofday(&tv, NULL);
				tsec = (uint64_t) tv.tv_sec;
				tusec = (uint64_t) tv.tv_usec;
				ts64 = (tsec*1000000 + tusec)*9/100;
				ts = (uint32_t) (ts64 & 0xffffffff);

				/* Run the registered audio callback */
				aud_cb(packet.buffer, packet.size,
					packet.format, ts, aud_cb_user_data, &param);
			}
			packet.size = 0;
		}
	}

#if DEBUG_AUDIO_CALLBACK > 0
	/* Check how long the callback ran */
	gettimeofday(&tv_end, &tz);
	time_counter++;

	delta_time = (1000000*tv_end.tv_sec + tv_end.tv_usec) -
		(1000000*tv_start.tv_sec + tv_start.tv_usec);

	if (delta_time > max_delta_time) {
		max_delta_time = delta_time;
	}

	if(time_counter > 50) {
		TRACE("Worst audio callback processing time = %i ms\n", max_delta_time/1000);
		time_counter = 0;
		max_delta_time = 0;
	}
#endif

	/* Resubmit the URB */
	if(aud_started && libusb_submit_transfer(transfer)>=0)
		incr_active_transfers();
	else
		free_audio_transfer(aud_cfg.xfers, xfer_idx);
}

/* Setup the ISOC transfer by allocating the packets for transfer and
 * the buffer for audio data. */
static int setup_isoc_transfer()
{
	uintptr_t i;
	static unsigned int packet_size;

	if (packet_size != aud_cfg.format[aud_cfg.fmt_idx].pkt_size) {
		packet_size = aud_cfg.format[aud_cfg.fmt_idx].pkt_size;
	}
	/* We allocate one more 'audio_duration_ms' transfer than necessary on purpose */
	/* It will be use later to handle audio wrapping */
	if(abuf == NULL)
		abuf = malloc(packets_per_transfer*packet_size*num_transfers
				+ audio_duration_ms*packet_size);

	if(aud_cfg.xfers == NULL)
		aud_cfg.xfers = calloc(num_transfers, sizeof(struct libusb_transfer*));

	for (i=0; i<num_transfers; i++) {
		aud_cfg.xfers[i] = libusb_alloc_transfer(packets_per_transfer);
		CHECK_ERROR(abuf == NULL || aud_cfg.xfers[i] == NULL,
			-1, "Unable to allocate ISOC libusb transfer");

		libusb_fill_iso_transfer(aud_cfg.xfers[i],
				audio_hdl,
				aud_cfg.format[aud_cfg.fmt_idx].ep,
				abuf + i*(packets_per_transfer * packet_size),
				packets_per_transfer * packet_size,
				packets_per_transfer,
				(libusb_transfer_cb_fn) aud_cap_callback,
				(void*)i,
				USB_TIMEOUT);

		libusb_set_iso_packet_lengths(aud_cfg.xfers[i], packet_size);
	}

	return 0;
}

/* Start the audio capture by selecting the alternate settings based on
 * current sample rate */
static int _mxuvc_audio_start(audio_channel_t ch)
{
	int i, ret;

	TRACE("Starting audio\n");

	CHECK_ERROR(ch >= audio_num_channels, -1,
			"Unsupported channel number %d", ch);

	CHECK_ERROR(aud_started == 1, -1, "Unable to start the audio: "
			"audio has already been started.");

	CHECK_ERROR(mxuvc_audio_alive() == 0, -1,
			"Unable to start the audio: the camera has been disconnected.");

	/* Initialize the USB transfers */
	ret = setup_isoc_transfer();
	CHECK_ERROR(ret < 0, -1, "Unable to setup the ISOC transfers.");

	/* Switch alternate setting */
	TRACE2("Using interface %d and alternate setting %d\n",
			aud_cfg.interface,
			aud_cfg.format[aud_cfg.fmt_idx].alt_set);

	ret = libusb_set_interface_alt_setting(audio_hdl,
			aud_cfg.interface,
			aud_cfg.format[aud_cfg.fmt_idx].alt_set);
	CHECK_ERROR(ret < 0, -1, "Unable to set the alternate setting "
				"(err %i)\n", ret);

	aud_first_transfer = 1;
	aud_started = 1;

	/* Submit the USB transfers */
	set_active_transfers(0);
	for (i=0; i<(int)num_transfers; i++) {
		ret = libusb_submit_transfer(aud_cfg.xfers[i]);
		if (ret < 0) {
			libusb_free_transfer(aud_cfg.xfers[i]);
			ERROR(-1, "Unable to submit ISOC audio "
				"transfer (err %i)", ret) ;
		}
		incr_active_transfers();
	}

	return ret;
}
int mxuvc_audio_start(audio_channel_t ch)
{
	RECORD("");
	return _mxuvc_audio_start(ch);
}

/* Stop the audio capture by selecting the alternate settings 0 */
static int _mxuvc_audio_stop(audio_channel_t ch)
{
	int ret;
	unsigned int c=0, i;

	TRACE("Stopping audio\n");

	CHECK_ERROR(ch >= audio_num_channels, -1,
			"Unsupported channel number %d", ch);

	CHECK_ERROR(aud_started == 0, -1, "Unable to stop the audio: "
			"audio hasn't been started yet.");

	aud_started = 0;

	if(!audio_disconnected) {
		/* First try to wait for the transfer to complete instead of canceling them.
		 * Cancel the transfers if it takes more than 1 seconds for the transfers
		 * to complete.
		 * The reason why we don't directly cancel the transfers is that in ISOC
		 * mode, libusb sometimes seems to miss packets on other interfaces while
		 * transfers are being canceled on this interface */
		while(get_active_transfers()) {
			if(c >= 1000000) {
				for(i=0; i<num_transfers; i++)
					if(aud_cfg.xfers[i] != NULL)
						libusb_cancel_transfer(aud_cfg.xfers[i]);
			}
			usleep(3000);
			c+=3000;
		}

		/* Select alt settings 0 */
		ret = libusb_set_interface_alt_setting(audio_hdl,
				aud_cfg.interface, 0);
		CHECK_ERROR(ret < 0, -1,
				"Unable to stop the audio: could not set the "
				"alternate setting.");
	} else {
		while(get_active_transfers())
			usleep(3000);
	}

	TRACE("%u audio frames captured\n", aud_frame_count);
	aud_frame_count = 0;

	return 0;
}
int mxuvc_audio_stop(audio_channel_t ch)
{
	RECORD("");
	return _mxuvc_audio_stop(ch);
}

int mxuvc_audio_set_format(audio_channel_t ch, audio_format_t fmt)
{
	RECORD("%s", audformat2str(fmt));

	CHECK_ERROR(ch >= audio_num_channels, -1,
			"Unsupported channel number %d", ch);

	TRACE("Setting audio format to %s.\n", audformat2str(fmt));

	switch(fmt) {
	case AUD_FORMAT_AAC_RAW:
		ERROR(-1, "Unable to set the audio format to AAC. Audio "
				"format not implemented");
		cur_aud_format = AUD_FORMAT_AAC_RAW;
		break;
	case AUD_FORMAT_PCM_RAW:
		cur_aud_format = AUD_FORMAT_PCM_RAW;
		break;
	default:
		ERROR(-1, "Unable to set the audio format: unknown audio "
				"format %i", fmt);
		break;
	}

	return 0;
}

/* Sets the sample rate */
int mxuvc_audio_set_samplerate(audio_channel_t ch, int samplingFr)
{
	RECORD("%i", samplingFr);
	int i, splr, restart=0, ret;

	CHECK_ERROR(ch >= audio_num_channels, -1,
			"Unsupported channel number %d", ch);

	TRACE("Setting audio sample rate to %i.\n", samplingFr);

	if(aud_started) {
		ret = _mxuvc_audio_stop(ch);
		CHECK_ERROR(ret < 0, -1,
				"Could not set the audio sample rate to %i",
				samplingFr);
		restart = 1;
	}

	/* Select alt settings [1:8khz] [2:16khz] [3:24khz] */
	for(i=0;i<MAX_AUD_FMTS;i++) {
		if(aud_cfg.format[i].samFr != samplingFr)
			continue;

		aud_cfg.fmt_idx = i;
		splr = aud_cfg.format[aud_cfg.fmt_idx].samFr;
		aud_target_size = AUD_NUM_CH*2*splr*audio_duration_ms/1000;
		if(restart) {
			ret = _mxuvc_audio_start(ch);
			CHECK_ERROR(ret < 0, -1,
					"Could not set the audio sample rate to %i",
					samplingFr);
		}
		return 0;
	}

	ERROR_NORET("Could not set the audio sample rate to %i: "
			"sample rate not available.", samplingFr);
	return -1;
}

/* Get the current sample rate */
int mxuvc_audio_get_samplerate(audio_channel_t ch)
{
	RECORD("");

	CHECK_ERROR(ch >= audio_num_channels, -1,
			"Unsupported channel number %d", ch);

	return aud_cfg.format[aud_cfg.fmt_idx].samFr;
}

/* Range from 0 to 100 */
int mxuvc_audio_set_volume(int vol)
{
	RECORD("%i", vol);
	int ret;
	int16_t uac_vol;
	AUDIO_CTRL *ctrl;

	TRACE("Setting audio volume to %i\n", vol);

	CHECK_ERROR(vol < 0 || vol > 100, -1, "Unable to set audio "
			"volume to %i. Volume must be in the range "
			"[0; 100].", vol);

	ctrl = get_ctrl_by_id(CTRL_VOLUME);
	uac_vol = (ctrl->max-ctrl->min)*vol/100 + ctrl->min;

	if(aud_cfg.vol != uac_vol){
		aud_cfg.vol = uac_vol;
		ret = set_ctrl(CTRL_VOLUME, SET_CUR, &uac_vol);
		CHECK_ERROR(ret < 0, -1, "Unable to set audio volume to %i",
				vol);
	} else {
		TRACE("Audio volume already set to %i\n", vol);
	}
	return 0;
}

/* Get current volume */
int mxuvc_audio_get_volume()
{
	RECORD("");
	return aud_cfg.vol;
}

int mxuvc_audio_set_mic_mute(int bMute)
{
	RECORD("%i", bMute);

	TRACE("Setting mic mute to %i\n", bMute);

	int16_t ctrlVal = ((int16_t) bMute) & 0x1;
	set_ctrl(CTRL_MUTE, SET_CUR, &ctrlVal);
	return 0;
}

int mxuvc_audio_register_cb(audio_channel_t ch, mxuvc_audio_cb_t func, void *user_data)
{
	RECORD("%p, %p",func, user_data);

	CHECK_ERROR(ch >= audio_num_channels, -1,
			"Unsupported channel number %d", ch);

	aud_cb = func;
	aud_cb_user_data = user_data;
	return 0;
}
