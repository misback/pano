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

#ifndef __ADESC_PARSE_H__
#define __ADESC_PARSE_H__

#include <stdint.h> /* uintX_t */
#include <libusb-1.0/libusb.h>

#include "mxuvc.h"

#define MAX_AUD_FMTS    8

struct aud_format {
	int ep;
	int alt_set;
	int samFr;
	unsigned int pkt_size;
	int access_unit;
};

struct audio_cfg {
	struct aud_format format[MAX_AUD_FMTS];
	int fmt_idx;
	int interface;
	int ctrlif;
	int ctrl_feature;
	int uframe_interval;
	int vol;
	unsigned int num_chan;
	struct libusb_transfer **xfers;
};

int aparse_usb_config(struct libusb_device *dev, uint8_t bConfigurationValue);

#endif  // #ifdef __ADESC_PARSE_H__
