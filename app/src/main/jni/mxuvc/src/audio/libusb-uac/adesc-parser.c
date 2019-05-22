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

#include "common.h"
#include "adesc-parser.h"

struct audio_cfg aud_cfg;

/******************
 *    UAC defs    *
 ******************/
// audio interface class codes
#define CC_AUDIO                0x01
#define CS_INTERFACE            0x24

// interface subclass codes
#define SC_SUBCLASS_UNDEFINED   0x00
#define SC_AUDIOCONTROL         0x01
#define SC_AUDIOSTREAMING       0x02

// audio interface protocol codes
#define PR_PROTOCOL_UNDEFINED   0x00

// audio interface subtypes
#define AC_DESCRIPTOR_UNDEFINED 0x00
#define AC_HEADER               0x01
#define AC_INPUT_TERMINAL       0x02
#define AC_OUTPUT_TERMINAL      0X03
#define AC_MIXER_UNIT           0x04
#define AC_SELECTOR_UNIT        0x05
#define AC_FEATURE_UNIT         0x06
#define AC_PROCESSING_UNIT      0x07
#define AC_EXTENSION_UNIT       0x08

// audio input terminal types
#define ITT_MICROPHONE          0x0201

// audio interface subtypes
#define AS_GENERAL              0x01
#define AS_FORMAT_TYPE          0x02
#define AS_FORMAT_SPECIFIC      0x03

struct uac_ctrl_interface_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint16_t bcdUAC;
	uint16_t wTotalLength;
	uint8_t bInCollection;
	uint8_t baInterfaceNr;
} PACKED;

struct uac_format_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bFormatType;
	uint8_t bNrChannels;
	uint8_t bSubFrameSize;
	uint8_t bBitResolution;
	uint8_t bSamFreqType;
	uint8_t tSamFreq[3];
} PACKED;

struct uac_streaming_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bTerminalLink;
	uint8_t bDelay;
	uint16_t wFormatTag;
} PACKED;

struct usb_endpoint_descriptor{
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint8_t  bEndpointAddress;
	uint8_t  bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t  bInterval;
} PACKED;

struct uac_input_terminal_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t bNrChannels;
	uint16_t wChannelConfig;
	uint8_t iChannelNames;
	uint8_t iTerminal;
} PACKED;

struct uac_feature_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bUnitID;
	uint8_t bSourceID;
	uint8_t bControlSize;
	uint8_t bmaControls[2];
	uint8_t iFeature;
} PACKED;

struct usb_descriptor_header {
	uint8_t bLength;
	uint8_t bDescriptorType;
} PACKED;



static int parse_usb_interface(const struct libusb_interface *interface);
static int parse_uac_ac(const struct libusb_interface_descriptor *if_desc);
static int parse_uac_as(const struct libusb_interface_descriptor *if_desc);

/*********************
 *    USB parsing    *
 *********************/
int aparse_usb_config(struct libusb_device *dev, uint8_t bConfigurationValue)
{
	struct libusb_config_descriptor *conf_desc;
	const struct libusb_interface *interface;
	int nint, ret;

	ret = libusb_get_config_descriptor_by_value(dev, bConfigurationValue,
							&conf_desc);
	CHECK_ERROR(ret < 0, ret,
			"Could not retreive configuration descriptor for "
			"configuration #%i", bConfigurationValue);

	TRACE2("bNumInterfaces = %i\n", conf_desc->bNumInterfaces);
	for(nint = 0; nint < conf_desc->bNumInterfaces; nint++) {
		interface = conf_desc->interface + nint;
		ret = parse_usb_interface(interface);
		if(ret < 0)
			return ret;
	}
	libusb_free_config_descriptor(conf_desc);
	return 0;
}

static int parse_usb_interface(const struct libusb_interface *interface)
{
	const struct libusb_interface_descriptor *altsetting;
	int nalt;
	int ret=0;

	TRACE2("\n### New interface ###\n");
	TRACE2("num_altsetting = %i\n", interface->num_altsetting);
	for(nalt = 0; nalt < interface->num_altsetting; nalt++) {
		altsetting = interface->altsetting + nalt;
		TRACE2("\n-- altsetting #%i, int #%i --\n",
				altsetting->bAlternateSetting,
				altsetting->bInterfaceNumber);

		switch(altsetting->bInterfaceClass) {
		case CC_AUDIO:
			switch(altsetting->bInterfaceSubClass) {
			case SC_AUDIOCONTROL:
				ret = parse_uac_ac(altsetting);
				break;
			case SC_AUDIOSTREAMING:
				ret = parse_uac_as(altsetting);
				break;
			default:
				ERROR(-1, "Unknown bInterfaceSubClass %i\n",
					altsetting->bInterfaceSubClass);
			}
			break;
		default:
			TRACE2("\n");
			break;
		}
		if(ret < 0)
			return ret;
	}
	return 0;
}

/*********************
 *    UAC parsing    *
 *********************/
static int parse_uac_ac(const struct libusb_interface_descriptor *if_desc)
{
	const struct uac_ctrl_interface_descriptor *hdr, *ac_desc;
	const struct uac_feature_unit_descriptor *feature_desc;
	const struct uac_input_terminal_descriptor *input_desc;
	int tot_len, len;

	TRACE2("audio control if : %d \n", if_desc->bInterfaceNumber);
	aud_cfg.ctrlif = if_desc->bInterfaceNumber;
	hdr = (const struct uac_ctrl_interface_descriptor *)if_desc->extra;

	CHECK_ERROR(hdr->bDescriptorType != CS_INTERFACE, -1,
			"Expected uac descriptor of type 0x%x but got type "
			"0x%x", CS_INTERFACE, hdr->bDescriptorType);

	CHECK_ERROR(hdr->bDescriptorSubType != AC_HEADER, -1,
			"Expected uac descriptor of subtype 0x%x but got "
			"type 0x%x", AC_HEADER, hdr->bDescriptorSubType);

	tot_len = hdr->wTotalLength;
	len = hdr->bLength;
	while(len < tot_len) {
		ac_desc = (const struct uac_ctrl_interface_descriptor *)
			(if_desc->extra + len);

		switch(ac_desc->bDescriptorSubType) {
		case AC_FEATURE_UNIT:
			feature_desc =
				(const struct uac_feature_unit_descriptor*) ac_desc;
			aud_cfg.ctrl_feature = feature_desc->bUnitID;
			TRACE2("aud_ctrl_feature id: %d \n",
					aud_cfg.ctrl_feature);
			break;
		case AC_INPUT_TERMINAL:
			input_desc =
				(const struct uac_input_terminal_descriptor*) ac_desc;
			aud_cfg.num_chan = input_desc->bNrChannels;
			TRACE2("Number of channels: %i \n",
					aud_cfg.num_chan);
			break;
		default:
			TRACE2("\n");
			break;
		}
		len += ac_desc->bLength;
	}

	return 0;
}


static int parse_uac_as(const struct libusb_interface_descriptor *if_desc)
{
	const struct uac_format_descriptor *uac_fmt_desc;
	const struct libusb_endpoint_descriptor *ep;

	int samFr;
	TRACE2("SC_AUDIOSTREAMING\n");

	CHECK_ERROR(aud_cfg.fmt_idx >= MAX_AUD_FMTS, -1,
			"No. of Aud configs exceeds MAX_AUD_FMTS !!");

	aud_cfg.interface = if_desc->bInterfaceNumber;
	if(if_desc->bAlternateSetting > 0) {

		uac_fmt_desc = (const struct uac_format_descriptor *)if_desc->extra;
		while(uac_fmt_desc->bDescriptorSubType != AS_FORMAT_TYPE){
			uac_fmt_desc = (const struct uac_format_descriptor *)
				(if_desc->extra + uac_fmt_desc->bLength);
			CHECK_ERROR(uac_fmt_desc->bDescriptorType == 0x5, -1,
					"Reached end of Interface decsriptor");
		}

		TRACE2("\tFound AS Format\n");
		samFr = uac_fmt_desc->tSamFreq[2];
		samFr = (samFr<<8) | uac_fmt_desc->tSamFreq[1];
		samFr = (samFr<<8) | uac_fmt_desc->tSamFreq[0];

		ep = (const struct libusb_endpoint_descriptor *)(if_desc->endpoint);
		aud_cfg.format[aud_cfg.fmt_idx].ep = ep->bEndpointAddress;
		aud_cfg.format[aud_cfg.fmt_idx].pkt_size =
							ep->wMaxPacketSize;
		TRACE2("\t\tep->>bEndpointAddress = 0x%x\n",
				ep->bEndpointAddress);
		SHOW2(ep->wMaxPacketSize);
		aud_cfg.uframe_interval = 1 >> (ep->bInterval-1);
		SHOW2(ep->bInterval);
		aud_cfg.format[aud_cfg.fmt_idx].alt_set =
						if_desc->bAlternateSetting;
		aud_cfg.format[aud_cfg.fmt_idx].samFr = samFr;
		SHOW2(samFr);
		aud_cfg.format[aud_cfg.fmt_idx].access_unit = samFr/100;
		aud_cfg.fmt_idx++;
	}

	return 0;
}
