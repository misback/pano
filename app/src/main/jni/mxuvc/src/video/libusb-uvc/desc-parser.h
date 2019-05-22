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

#ifndef __DESC_PARSE_H__
#define __DESC_PARSE_H__

#include <stdint.h> /* uintX_t */
#include <libusb/libusb.h>
#include <pthread.h>

#include "mxuvc.h"

#define MAX_ALT_PER_INT 32

/* VS subtypes */
#define VS_UNDEFINED                    0x00
#define VS_INPUT_HEADER                 0x01
#define VS_FORMAT_UNCOMPRESSED          0x04
#define VS_FRAME_UNCOMPRESSED           0x05
#define VS_FORMAT_MJPEG                 0x06
#define VS_FRAME_MJPEG                  0x07
#define VS_FORMAT_MPEG2TS               0x0a
#define VS_COLORFORMAT                  0x0d
#define VS_FORMAT_FRAME_BASED           0x10
#define VS_FRAME_FRAME_BASED            0x11

typedef enum {
	FRI_DISCRETE = 0,
	FRI_CONTINUOUS = 1
} fri_t;

struct frame_interval {
	fri_t type;

	/* For continuous fri */
	unsigned int min;
	unsigned int max;
	unsigned int step;

	/* For discrete fri */
	unsigned int num;
	uint32_t *discretes;
};

struct frame {
	unsigned int type;
	unsigned int id;

	unsigned int width;
	unsigned int height;

	unsigned int bitrate_min;
	unsigned int bitrate_max;

	unsigned int default_fri;
	struct frame_interval fri;

	/* For MJPG and Uncompressed frame types */
	unsigned int buffer_size;
};

struct format {
	unsigned int type;
	unsigned int id;

	/* For frame based format */
	unsigned int num_frames;
	unsigned int default_frame_id;
	uint8_t guid_format[16];
	struct frame *frames;

	/* For stream based format */
	unsigned int packet_size;
};

struct control_unit_std {
	unsigned int id;
	uint64_t bmControls;
};
struct control_unit_ext {
	unsigned int id;
	/* Assume that bitmap max size is 64 bits */
	uint64_t bmControls;
	uint8_t guid[16];
	struct control_unit_ext *next;
};

struct video_cfg {
	int enabled;

	struct {
		unsigned int interface;
		struct control_unit_std processing_unit;
		struct control_unit_std camera_unit;
		struct control_unit_ext *extension_unit;
		unsigned int num_extension_unit;
	} vc;

	struct {
		unsigned int interface;
		unsigned int num_formats;
		struct format *formats;
		int bulk;

		/* Number of alternate settings for video streaming */
		unsigned int num_video_alt;
		/* wMaxPacketSize and endpoint for each alternate settings.
		 * Index starts at 1. */
		unsigned int alt_maxpacketsize[MAX_ALT_PER_INT+1];
		unsigned int alt_ep[MAX_ALT_PER_INT+1];
	} vs;


};

struct video_settings {
	struct libusb_transfer **xfers;
	int started;                     /* 1 = streaming */
	int first_transfer;              /* Video just started. This flag
                                        needs to be cleared after the
                                        first USB transfer is received */

	video_format_t cur_video_format; /* Current video format */
	struct format *cur_format;       /* Current UVC format */
	struct frame  *cur_frame;        /* Current UVC frame */
	unsigned int   cur_fri;          /* Current frame interval */
	unsigned int   cur_alt;          /* Current alternate setting */
	unsigned int   frame_count;
	unsigned int   max_transfer_size; /* = dwMaxPayloadTransferSize: maximum
	                                       size of a UVC 'payload' */
	unsigned int   max_frame_size;    /* = dwMaxVideoFrameSize: maximum size
	                                       of a video frame or codec segment */

	mxuvc_video_cb_t cb;                   /* Callback function */
	void *cb_user_data;              /* User data pointer for callback */
	int quirk_demuxts;               /* Hack for Raptor: demux ts on host */
	int mux_aac_ts;                  /* AAC muxing enabled?
                                            (AAC + H264 in TS) */
	int32_t cache_pan;
	int32_t cache_tilt;
	pthread_mutex_t pantilt_mutex;
};

int parse_usb_config(struct libusb_device *dev, uint8_t bConfigurationValue);

#endif  // #ifdef __DESC_PARSE_H__
