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
#include <stdlib.h> /* malloc() */
#include <libusb-1.0/libusb.h>
#include <endian.h> /* htole32() */
#include "libusb/handle_events.h"
#include "mxuvc.h"
#include "common.h"
#include <unistd.h>

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#include <linux/byteorder/big_endian.h>
#else
#include <linux/byteorder/little_endian.h>
#endif

struct audio_process_params_t
{
	int channel; /* channel to which the params to set */
	AUDIO_FILTER_PARAMS param;
	int value;
} __attribute__((__packed__));

static struct libusb_device_handle *camera = NULL;
static struct libusb_context *ctxt = NULL;

/* initialize custom control plugin */
int mxuvc_custom_control_init(void)
{
	int ret=0, i=0;
	struct libusb_device **devs=NULL;
	struct libusb_device *dev;
	struct libusb_device_handle *devhandle = NULL;

	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *conf_desc;
	const struct libusb_interface *dev_interface;
	const struct libusb_interface_descriptor *altsetting;
	int scan_result = 0;

	if (camera == NULL){
		ret = init_libusb(&ctxt);
		if (ret) {
			TRACE("libusb_init failed\n");
			return -1;
		}
		//scan and detect GEO devices
		if (libusb_get_device_list(ctxt, &devs) < 0)
		{
			TRACE("libusb_get_device_list error\n");
			return -1;	
		}
		while ((dev = devs[i++]) != NULL) {
			int data[2] = {-1, -1};

			ret = libusb_get_device_descriptor(dev, &desc);
			if (ret < 0)
				continue;
			ret = libusb_get_config_descriptor_by_value(dev, 1, &conf_desc);
			if(ret < 0)
				continue;

			dev_interface = conf_desc->interface;		
			altsetting = dev_interface->altsetting;
			/* We are only interested in devices whose first USB class is
			 *  - a Vendor specific class
			 *  - a UVC class
			 * */
			if (altsetting->bInterfaceClass != VENDOR_SPECIFIC
					&& altsetting->bInterfaceClass != CC_VIDEO) {
				libusb_free_config_descriptor(conf_desc);
				continue;
			}

			/* Open the device to communicate with it */
			ret = libusb_open(dev, &devhandle);
			if (ret < 0) {
				libusb_free_config_descriptor(conf_desc);
				continue;
			}
			ret = libusb_control_transfer(devhandle,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ CMD_WHO_R_U,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&data,
				/* wLength       */ 8,
				/* timeout*/  LIBUSB_CMD_TIMEOUT
				);

			switch(data[0]) {
				case MAX64380:
				case MAX64480:
				case MAX64580:
					//got the Geo camera
					scan_result = 1;
				break;
				default:
				break;	
			}
			if(scan_result == 1){
				if(data[1] != 0)
					scan_result = 0;
			}

			if(scan_result == 1)
			{
				libusb_free_config_descriptor(conf_desc);
				break;
			}
			else
				libusb_close(devhandle);
					
			libusb_free_config_descriptor(conf_desc);
		}
		libusb_free_device_list(devs, 1);

		if(scan_result == 1)	
			camera = devhandle;	
		else {
			TRACE("ERR: Opening camera failed\n");
			return -1;
		}
		//camera = libusb_open_device_with_vid_pid(ctxt, 0x0b6a, 0x4d52);
		if (camera == NULL) {
			TRACE("ERR: Opening camera failed\n");
			return -1;
		}
		TRACE("Custom control Initerface init done\n");
	}

	return ret;
}

int mxuvc_custom_control_deinit(void)
{
	if (camera){
		libusb_close (camera);
		exit_libusb(&ctxt);
		ctxt = NULL;
		camera = NULL;
	}

	return 0;		
}

int mxuvc_custom_control_set_vad(uint32_t vad_status)
{
	int ret = 0;

	if(camera){
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ AUDIO_VAD,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&vad_status,
				/* wLength       */ sizeof(uint32_t),
				/* timeout*/   LIBUSB_CMD_TIMEOUT 
				);
			if (ret < 0) {
				TRACE("ERROR: Send Vad Status failed\n");
				return -1;
			}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}

	return ret;
}

int mxuvc_custom_control_enable_aec(void)
{
	int ret=0;

	if(camera){
		TRACE("Set AEC Enable\n");
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ AEC_ENABLE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ NULL,
				/* wLength       */ 0,
				/* timeout*/   LIBUSB_CMD_TIMEOUT
				);
		if (ret < 0) {
			TRACE("ERROR: AEC Enable failed\n");
			return -1;
		}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}

	return 0;
}

int mxuvc_custom_control_disable_aec(void)
{
	int ret=0;

	if(camera){
		TRACE("Set AEC Disable\n");
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ AEC_DISABLE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ NULL,
				/* wLength       */ 0,
				/* timeout*/   LIBUSB_CMD_TIMEOUT 
				);
		if (ret < 0) {
			TRACE("ERROR: AEC Disbale failed\n");
			return -1;
		}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}

	return 0;
}

int mxuvc_custom_control_set_audio_codec_samplerate(unsigned int samplerate)
{
	int ret=0;

	if(camera){
		TRACE("Set Audio Codec Samplerate to %d\n",samplerate);
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ AEC_SET_SAMPLERATE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&samplerate,
				/* wLength       */ sizeof(unsigned int),
				/* timeout*/   LIBUSB_CMD_TIMEOUT 
				);
		if (ret < 0) {
			TRACE("ERROR: Set AEC samplerate failed\n");
			return -1;
		}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}

	return 0;
}

int mxuvc_custom_control_set_audclk_mode(audio_channel_t ch, audclk_mode_t mode)
{
	int ret=0;

	if(camera){
		TRACE("Set AudioClock mode to %d\n",mode);
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ AUDCLK_MODE_SET,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&mode,
				/* wLength       */ sizeof(unsigned int),
				/* timeout*/   LIBUSB_CMD_TIMEOUT
				);
		if (ret < 0) {
			TRACE("ERROR: Set AudioClock mode failed\n");
			return -1;
		}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}
	return 0;
}

int mxuvc_custom_control_get_audclk_mode(audio_channel_t ch, audclk_mode_t *mode)
{
	int ret=0;
	if(camera){
		TRACE("Get AudioClock mode\n");
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ AUDCLK_MODE_GET,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)mode,
				/* wLength       */ sizeof(unsigned int),
				/* timeout*/   LIBUSB_CMD_TIMEOUT 
				);
		if (ret < 0) {
			TRACE("ERROR: Get AudioClock mode failed\n");
			return -1;
		}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}
	return 0;
}

int mxuvc_custom_control_set_spkr_samplerate(uint32_t samplerate)
{
	int ret=0;

	samplerate = __cpu_to_le32(samplerate);

	if(camera){
		TRACE("Set Speaker Samplerate to %d\n", samplerate);
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ SET_SPKR_SAMPLING_RATE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&samplerate,
				/* wLength       */ sizeof(uint32_t),
				/* timeout*/   LIBUSB_CMD_TIMEOUT
				);
		if (ret < 0) {
			TRACE("ERROR: Set speaker samplerate failed\n");
			return -1;
		}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}

	return 0;
}

int mxuvc_custom_control_get_spkr_samplerate(uint32_t *samplerate)
{
	int ret=0;

	if(camera){
		TRACE("Get Speaker Samplerate\n");
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ GET_SPKR_SAMPLING_RATE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)samplerate,
				/* wLength       */ sizeof(uint32_t),
				/* timeout*/   LIBUSB_CMD_TIMEOUT
				);
		if (ret < 0) {
			TRACE("ERROR: Get speaker samplerate failed\n");
			return -1;
		}
		*samplerate = __le32_to_cpu(*samplerate);
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}

	return 0;
}

int mxuvc_custom_control_set_spkr_gain(uint32_t gain)
{
	int ret=0;

	gain = __cpu_to_le32(gain);

	if(camera){
		TRACE("Set speaker gain to %d\n", gain);
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ SET_SPKR_VOL,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&gain,
				/* wLength       */ sizeof(uint32_t),
				/* timeout*/   LIBUSB_CMD_TIMEOUT
				);
		if (ret < 0) {
			TRACE("ERROR: Set speaker gain failed\n");
			return -1;
		}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}

	return 0;
}

int mxuvc_custom_control_get_spkr_gain(uint32_t *gain)
{
	int ret=0;
	if(camera){
		TRACE("Get speaker gain\n");
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ GET_SPKR_VOL,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)gain,
				/* wLength       */ sizeof(uint32_t),
				/* timeout*/   LIBUSB_CMD_TIMEOUT
				);
		if (ret < 0) {
			TRACE("ERROR: Get speaker gain failed\n");
			return -1;
		}
		*gain = __le32_to_cpu(*gain);
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}
	return 0;
}

int mxuvc_custom_control_enable_spkr(void)
{
	int ret=0;

	if(camera){
		TRACE("Set Spkr Enable\n");
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ SPKR_ENABLE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ NULL,
				/* wLength       */ 0,
				/* timeout*/   LIBUSB_CMD_TIMEOUT
				);
		if (ret < 0) {
			TRACE("ERROR: Spkr Enable failed\n");
			return -1;
		}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}

	return 0;
}

int mxuvc_custom_control_disable_spkr(void)
{
	int ret=0;

	if(camera){
		TRACE("Set Spkr Disable\n");
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ SPKR_DISABLE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ NULL,
				/* wLength       */ 0,
				/* timeout*/   LIBUSB_CMD_TIMEOUT
				);
		if (ret < 0) {
			TRACE("ERROR: Spkr Disbale failed\n");
			return -1;
		}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}

	return 0;
}


int mxuvc_custom_control_get_audio_stats(void *audio_stats)
{
	int ret=0;

	if(camera){
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ 0x2A, //AUDIO_STATS
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)audio_stats,
				/* wLength       */ sizeof(asp_metadata),
				/* timeout*/   LIBUSB_CMD_TIMEOUT
				);
			if (ret < 0) {
				TRACE("ERROR: Audio stats request failed\n");
				return -1;
			}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}

	return 0;
}


int mxuvc_custom_control_enable_agc(void)
{
	int ret=0;

	if(camera){
		TRACE("Set AGC On\n");
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ AGC_ENABLE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ NULL,
				/* wLength       */ 0,
				/* timeout*/   LIBUSB_CMD_TIMEOUT
				);
		if (ret < 0) {
			TRACE("ERROR: AGC On failed\n");
			return -1;
		}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}

	return 0;
}

int mxuvc_custom_control_disable_agc(void)
{
	int ret=0;

	if(camera){
		TRACE("Set AGC Off\n");
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ AGC_DISABLE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ NULL,
				/* wLength       */ 0,
				/* timeout*/   LIBUSB_CMD_TIMEOUT
				);
		if (ret < 0) {
			TRACE("ERROR: AGC Off failed\n");
			return -1;
		}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}

	return 0;
}

int mxuvc_custom_control_enable_asp(audio_channel_t ch)
{
	int ret=0;
	struct audio_process_params_t audio_filter;

	if(ch != AUD_CH1)
	{
		TRACE("%s:ERROR-> Supported only for PCM channel",__func__);
		return -1;
	}

	memset(&audio_filter, 0, sizeof(struct audio_process_params_t));

	audio_filter.channel = __cpu_to_le32(1);
	audio_filter.param   = __cpu_to_le32(ASP);
	audio_filter.value   = __cpu_to_le32(1);

	if(camera){
		TRACE("Set ASP On\n");
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ AUDIO_FILTER_PARAM,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char*) &audio_filter,
				/* wLength       */ sizeof(struct audio_process_params_t),
				/* timeout*/   LIBUSB_CMD_TIMEOUT
				);

		if (ret < 0) {
			TRACE("ERROR: ASP On failed\n");
			return -1;
		}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}

	return 0;
}

int mxuvc_custom_control_disable_asp(audio_channel_t ch)
{
	int ret=0;
	struct audio_process_params_t audio_filter;

	if(ch != AUD_CH1)
	{
		TRACE("%s:ERROR-> Supported only for PCM channel",__func__);
		return -1;
	}

	memset(&audio_filter, 0, sizeof(struct audio_process_params_t));

	audio_filter.channel = __cpu_to_le32(1);
	audio_filter.param   = __cpu_to_le32(ASP);
	audio_filter.value   = __cpu_to_le32(0);

	if(camera){
		TRACE("Set ASP Off\n");
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ AUDIO_FILTER_PARAM,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char*) &audio_filter,
				/* wLength       */ sizeof(struct audio_process_params_t),
				/* timeout*/   LIBUSB_CMD_TIMEOUT
				);
		if (ret < 0) {
			TRACE("ERROR: ASP Off failed\n");
			return -1;
		}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}

	return 0;
}

#define QPARAM_MAX_STRING_SIZE 64 // Keep it a multiple of 4

struct set_qparam
{
    char object[QPARAM_MAX_STRING_SIZE];
    char name[QPARAM_MAX_STRING_SIZE];
    char type[QPARAM_MAX_STRING_SIZE];
    int  value;
    char activate;
} __attribute__((__packed__));

int mxuvc_custom_control_set_qparam(const char *object_name,
                                    const char *param_type,
                                    const char *param_name,
                                    int param_value,
                                    char activate_cfg)
{
    int ret, object_name_len, param_name_len, type_name_len;
    struct set_qparam qparam;
    int cmd_sta = 0, retry = 10;

    object_name_len = strlen(object_name) + 1;
    param_name_len  = strlen(param_name) + 1;
    type_name_len  = strlen(param_type) + 1;

    CHECK_ERROR(object_name_len > QPARAM_MAX_STRING_SIZE, -1,
                "QParam object name must be strictly less than %i bytes",
                QPARAM_MAX_STRING_SIZE);
    CHECK_ERROR(param_name_len > QPARAM_MAX_STRING_SIZE, -1,
                "QParam parameter name must be stricly less than %i bytes",
                QPARAM_MAX_STRING_SIZE);
    CHECK_ERROR(type_name_len > QPARAM_MAX_STRING_SIZE, -1,
                "QParam type name must be stricly less than %i bytes",
                QPARAM_MAX_STRING_SIZE);

    memset(&qparam, 0, sizeof(struct set_qparam));
    memcpy(qparam.object, object_name, object_name_len);
    memcpy(qparam.name,   param_name,  param_name_len);
    memcpy(qparam.type,   param_type,  type_name_len);
    qparam.value = __cpu_to_le32(param_value);
    qparam.activate = activate_cfg;

    CHECK_ERROR(!camera, -1,
                "%s: Custom Control Plug-in is not enabled",__func__);

    ret = libusb_control_transfer(camera,
                                  /* bmRequestType */
                                  (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                                   LIBUSB_RECIPIENT_INTERFACE),
                                  /* bRequest      */ SET_QPARAM,
                                  /* wValue        */ 0,
                                  /* MSB 4 bytes   */
                                  /* wIndex        */ 0,
                                  /* Data          */ (unsigned char*) &qparam,
                                  /* wLength       */ sizeof(struct set_qparam),
                                  /* timeout       */ LIBUSB_CMD_TIMEOUT);

    CHECK_ERROR(ret < 0, -1, "Failed SET_QPARAM\n");


    while(retry--) { 
        usleep(10*1000);
        ret = libusb_control_transfer(camera,
                                /* bmRequestType */
                                (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                                 LIBUSB_RECIPIENT_INTERFACE),
                                /* bRequest      */ SET_QPARAM_STATUS,
                                /* wValue        */ 0,
                                /* wIndex        */ 0,
                                /* Data          */ (unsigned char *)&cmd_sta,
                                /* wLength       */ sizeof(int),
                                /* timeout       */ LIBUSB_CMD_TIMEOUT
                                );

        CHECK_ERROR(ret < 0, -1, "Failed SET_QPARAM_STATUS\n");
        if (cmd_sta == 0) {
            break;
        }
    }
    return 0;
}

int mxuvc_custom_control_set_IRCF(ircf_state_t state, unsigned int time_ms)
{
    int ret=0;

	if(camera){
		TRACE("Set IRCF\n");
        if((state != IRCF_ENABLE) && (state != IRCF_DISABLE)){
            TRACE("ERROR: Invalid state %d\n",state);
            return -1;
        }
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ IRCF_SET,
				/* wValue        */ state,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)&time_ms,
				/* wLength       */ sizeof(unsigned int),
				/* timeout*/   LIBUSB_CMD_TIMEOUT
				);
		if (ret < 0) {
			TRACE("ERROR: Set IRCF failed\n");
			return -1;
		}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}

    return 0;
}

int mxuvc_custom_control_get_IRCF_state(ircf_state_t *state)
{
    int ret=0;

	if(camera){
		TRACE("Get IRCF state\n");
		ret = libusb_control_transfer(camera,
				/* bmRequestType */
				(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_INTERFACE),
				/* bRequest      */ IRCF_GET_STATE,
				/* wValue        */ 0,
				/* MSB 4 bytes   */
				/* wIndex        */ 0,
				/* Data          */ (unsigned char *)state,
				/* wLength       */ sizeof(ircf_state_t),
				/* timeout*/   LIBUSB_CMD_TIMEOUT
				);
		if (ret < 0) {
			TRACE("ERROR: Get IRCF state failed\n");
			return -1;
		}
	} else {
		TRACE("%s:ERROR-> Custom Control Plug-in is not enabled",__func__);
		return -1;
	}
    return 0;
}
