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
#include <string.h> /* memcpy */
#include <memory.h>

#include "common.h"
#include "desc-parser.h"

struct video_cfg       mxuvc_vcfg[NUM_VID_CHANNEL];
struct video_settings  mxuvc_vset[NUM_VID_CHANNEL];

#define EP_TYPE_MASK    0x3

#define EP_TYPE_CONTROL 0x0
#define EP_TYPE_ISOC    0x1
#define EP_TYPE_BULK    0x2
#define EP_TYPE_INT     0x3

/******************
 *    UVC defs    *
 ******************/
// video interface class codes
#define CC_VIDEO 0x0e

// interface subclass codes
#define SC_UNDEFINED			0x00
#define SC_VIDEOCONTROL			0x01
#define SC_VIDEOSTREAMING		0x02
#define SC_VIDEO_INTERFACE_COLLECTION	0x03

// video interface protocol codes
#define PC_PROTOCOL_UNDEFINED 0x00

// video descriptor types
#define CS_UNDEFINED		0x20
#define CS_DEVICE		0x21
#define CS_CONFIGURATION	0x22
#define CS_STRING		0x23
#define CS_INTERFACE		0x24
#define CS_ENDPOINT		0x25

// video descriptor subtypes
#define VC_DESCRIPTOR_UNDEFINED    0x00
#define VC_HEADER                  0x01
#define VC_INPUT_TERMINAL          0x02
#define VC_OUTPUT_TERMINAL         0x03
#define VC_SELECTOR_UNIT           0x04
#define VC_PROCESSING_UNIT         0x05
#define VC_EXTENSION_UNIT          0x06

// video input terminal types
#define ITT_VENDOR_SPECIFIC		0x0200
#define ITT_CAMERA			0x0201
#define ITT_MEDIA_TRANSPORT_INPUT	0x0202

// video terminal types
#define TT_VENDOR_SPECIFIC	0x0100
#define TT_STREAMING		0x0101

struct uvc_ctrl_interface_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint16_t bcdUVC;
	uint16_t wTotalLength;
	uint32_t dwClockFrequency;
	uint8_t bInCollection;
	uint8_t baInterfaceNr;
} PACKED;

struct uvc_camera_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t iTerminal;
	uint16_t wObjectiveFocalLengthMin;
	uint16_t wObjectiveFocalLengthMax;
	uint16_t wOcularFocalLength;
	uint8_t bControlSize;
	uint8_t bmControls0;
	uint8_t bmControls1;
	uint8_t bmControls2;
} PACKED;

struct uvc_output_terminal_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t bSourceID;
	uint8_t iTerminal;
} PACKED;

struct uvc_processing_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bUnitID;
	uint8_t bSourceID;
	uint16_t wMaxMultiplier;
	uint8_t bControlSize;
	uint8_t bmControls0;
	uint8_t bmControls1;
	uint8_t bmControls2;
	uint8_t iProcessing;
#ifdef UVC101
	uint8_t bmVideoStandards;
#endif
} PACKED;

struct uvc_extension_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bUnitID;
	uint8_t guidExtensionCode[16];
	uint8_t bNumControls;
	uint8_t bNrInPins;
	uint8_t baSourceID;
	uint8_t bControlSize;
	uint32_t bmControls;
	uint8_t iExtension;
} PACKED;

struct uvc_endpoint_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint16_t wMaxTransferSize;
} PACKED;

struct uvc_input_header_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bNumFormats;
	uint16_t wTotalLength;
	uint8_t bEndpointAddress;
	uint8_t bmInfo;
	uint8_t bTerminalLink;
	uint8_t bStillCaptureMethod;
	uint8_t bTriggerSupport;
	uint8_t bTriggerUsage;
	uint8_t bControlSize;
	uint8_t bmaControls[];


} PACKED;

struct uvc_mp2ts_format_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bFormatIndex;
	uint8_t bDataOffset;
	uint8_t bPacketLength;
	uint8_t bStrideLength;
#ifdef UVC101
	uint8_t guidStrideFormat[16];
#endif
} PACKED;

struct uvc_mjpeg_format_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bFormatIndex;
	uint8_t bNumFrameDescriptors;
	uint8_t bmFlags;
	uint8_t bDefaultFrameIndex;
	uint8_t bAspectRatioX;
	uint8_t bAspectRatioY;
	uint8_t bmInterlaceFlags;
	uint8_t bCopyProtect;
} PACKED;

struct uvc_mjpeg_frame_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bFrameIndex;
	uint8_t bmCapabilities;
	uint16_t wWidth;
	uint16_t wHeight;
	uint32_t dwMinBitRate;
	uint32_t dwMaxBitRate;
	uint32_t dwMaxVideoFrameBufferSize;
	uint32_t dwDefaultFrameInterval;
	uint8_t bFrameIntervalType;
	uint32_t dwFrameInterval[5];
} PACKED;

struct uvc_uncompressed_format_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bFormatIndex;
	uint8_t bNumFrameDescriptors;
	uint8_t guidFormat[16];
	uint8_t bBitsPerPixel;
	uint8_t bDefaultFrameIndex;
	uint8_t bAspectRatioX;
	uint8_t bAspectRatioY;
	uint8_t bmInterlaceFlags;
	uint8_t bCopyProtect;
} PACKED;

struct uvc_uncompressed_frame_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bFrameIndex;
	uint8_t bmCapabilities;
	uint16_t wWidth;
	uint16_t wHeight;
	uint32_t dwMinBitRate;
	uint32_t dwMaxBitRate;
	uint32_t dwMaxVideoFrameBufferSize;
	uint32_t dwDefaultFrameInterval;
	uint8_t bFrameIntervalType;
	uint32_t dwFrameInterval[5];
} PACKED;

struct uvc_frame_based_format_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bFormatIndex;
	uint8_t bNumFrameDescriptors;
	uint8_t guidFormat[16];
	uint8_t bBitsPerPixel;
	uint8_t bDefaultFrameIndex;
	uint8_t bAspectRatioX;
	uint8_t bAspectRatioY;
	uint8_t bmInterlaceFlags;
	uint8_t bCopyProtect;
	uint8_t bVariableSize;
} PACKED;

struct uvc_frame_based_frame_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bFrameIndex;
	uint8_t bmCapabilities;
	uint16_t wWidth;
	uint16_t wHeight;
	uint32_t dwMinBitRate;
	uint32_t dwMaxBitRate;
	uint32_t dwDefaultFrameInterval;
	uint8_t bFrameIntervalType;
	uint32_t dwBytesPerLine;
	uint32_t dwFrameInterval[3];
} PACKED;

static int parse_usb_interface(const struct libusb_interface *interface);
static int parse_uvc_vc(const struct libusb_interface_descriptor *altsetting);
static int parse_uvc_vs(const struct libusb_interface_descriptor *altsetting);
static int parse_uvc_format(const unsigned char *desc, struct format *format,
		unsigned int *num_frames);
static int parse_uvc_frame(const unsigned char *desc, struct frame *frame);
static int parse_uvc_vs_alt(const struct libusb_interface_descriptor *altsetting);
static video_channel_t chan = FIRST_VID_CHANNEL;
static struct video_cfg *cfg;
/*********************
 *    USB parsing    *
 *********************/
int parse_usb_config(struct libusb_device *dev, uint8_t bConfigurationValue)
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
	chan = FIRST_VID_CHANNEL;
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
		case CC_VIDEO:
			switch(altsetting->bInterfaceSubClass) {
			case SC_VIDEOCONTROL:
				ret = parse_uvc_vc(altsetting);
				break;
			case SC_VIDEOSTREAMING:
				if (altsetting->bAlternateSetting == 0)
				/* Video streaming interface alt 0 is the alt
				 * providing all the UVC video settings */
					ret = parse_uvc_vs(altsetting);
				else
				/* Video streaming interface alt > 0 is the
				 * list of alt settings  used for video
				 * streaming */
					ret = parse_uvc_vs_alt(altsetting);
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
 *    UVC parsing    *
 *********************/
static int parse_uvc_vc(const struct libusb_interface_descriptor *altsetting)
{
	const struct uvc_ctrl_interface_descriptor *vc_int_hdr;
	const struct uvc_ctrl_interface_descriptor *vc_int;
	const struct uvc_camera_descriptor *vc_cam;
	const struct uvc_processing_unit_descriptor *vc_pu;
	const struct uvc_extension_unit_descriptor *vc_xu;
	struct control_unit_ext *ext_unit;
	static struct control_unit_ext *last_ext_unit = NULL;

	int total_len, cur_len, subtype, i;
	char *bitmap;

	CHECK_ERROR(chan >= NUM_VID_CHANNEL, -1,
			"More channels than expected (%i) detected.",
			NUM_VID_CHANNEL);

	cfg = &(mxuvc_vcfg[chan]);
	cfg->enabled = 1;
	cfg->vc.interface = altsetting->bInterfaceNumber;

	TRACE2("SC_VIDEOCONTROL\n");
	vc_int_hdr = (const struct uvc_ctrl_interface_descriptor *) altsetting->extra;

	CHECK_ERROR(vc_int_hdr->bDescriptorType != CS_INTERFACE, -1,
			"Expected uvc descriptor of type 0x%x but got type "
			"0x%x", CS_INTERFACE, vc_int_hdr->bDescriptorType);

	CHECK_ERROR(vc_int_hdr->bDescriptorSubType != VC_HEADER, -1,
			"Expected uvc descriptor of subtype 0x%x but got "
			"type 0x%x", VC_HEADER,
			vc_int_hdr->bDescriptorSubType);

	TRACE2("VC_HEADER\n");
	total_len = vc_int_hdr->wTotalLength;
	cur_len = vc_int_hdr->bLength;
	while (cur_len < total_len) {
		vc_int = (const struct uvc_ctrl_interface_descriptor *)
				(altsetting->extra + cur_len);
		if(vc_int->bLength == 0) {
			WARNING("Video Control descriptor length cannot "
				"be 0. Check the descriptor sizes.");
			break;
		}
		subtype = vc_int->bDescriptorSubType;
		switch(subtype) {
		case VC_INPUT_TERMINAL:
			TRACE2("VC_INPUT_TERMINAL\n");
			vc_cam = (const struct uvc_camera_descriptor*) vc_int;
			cfg->vc.camera_unit.id = vc_cam->bTerminalID;
			bitmap = (char *) &cfg->vc.camera_unit.bmControls;
			bitmap[0] = vc_cam->bmControls0;
			bitmap[1] = vc_cam->bmControls1;
			bitmap[2] = vc_cam->bmControls2;
			break;
		case VC_PROCESSING_UNIT:
			TRACE2("VC_PROCESSING_UNIT\n");
			vc_pu = (const struct uvc_processing_unit_descriptor*) vc_int;
			cfg->vc.processing_unit.id = vc_pu->bUnitID;
			bitmap = (char *) &cfg->vc.processing_unit.bmControls;
			bitmap[0] = vc_pu->bmControls0;
			bitmap[1] = vc_pu->bmControls1;
			bitmap[2] = vc_pu->bmControls2;
			break;
		case VC_EXTENSION_UNIT:
			cfg->vc.num_extension_unit++;
			ext_unit = malloc(sizeof(struct control_unit_ext));
			ext_unit->next = NULL;
			if (last_ext_unit == NULL)
				cfg->vc.extension_unit = ext_unit;
			else
				last_ext_unit->next = ext_unit;

			last_ext_unit = ext_unit;
			TRACE2("VC_EXTENSION_UNIT\n");
			vc_xu = (const struct uvc_extension_unit_descriptor*) vc_int;
			ext_unit->id = vc_xu->bUnitID;
			ext_unit->bmControls = (uint64_t) vc_xu->bmControls;
			TRACE2("\tguid = ");
			for(i=0; i<16; i++) {
				ext_unit->guid[i]=vc_xu->guidExtensionCode[i];
				TRACE2("0x%x ", vc_xu->guidExtensionCode[i]);
			}
			TRACE2("\n");
			break;
		case VC_OUTPUT_TERMINAL:
			TRACE2("VC_OUTPUT_TERMINAL\n");
			break;
		case VC_SELECTOR_UNIT:
			TRACE2("VC_SELECTOR_UNIT\n");
			break;
		default:
			WARNING("Unknown VideoControl descriptor: "
				"sub type = 0x%x, length = %i.",
				subtype, vc_int->bLength);
		}
		cur_len += vc_int->bLength;
	}
	chan++;
	last_ext_unit = NULL;
	return 0;
}

static int parse_uvc_vs(const struct libusb_interface_descriptor *altsetting)
{
	int ret = 0;
	unsigned int num_frames = 0;
	int cur_format = -1;
	int cur_frame = -1;
	struct format *formats;
	struct frame *frames = NULL;
	const struct uvc_input_header_descriptor *vs_int_hdr;
	const struct uvc_input_header_descriptor *vs_int;
	int total_len, cur_len, subtype;

	TRACE2("SC_VIDEOSTREAMING\n");

	vs_int_hdr = (const struct uvc_input_header_descriptor *) altsetting->extra;

	CHECK_ERROR(vs_int_hdr == NULL, -1, "Input header descriptor not found");

	CHECK_ERROR(vs_int_hdr->bDescriptorType != CS_INTERFACE, -1,
			"Expected uvc descriptor of type 0x%x but got type "
			"0x%x", CS_INTERFACE, vs_int_hdr->bDescriptorType);

	CHECK_ERROR(vs_int_hdr->bDescriptorSubType != VS_INPUT_HEADER, -1,
			"Expected uvc descriptor of subtype 0x%x but got "
			"type 0x%x", VS_INPUT_HEADER,
			vs_int_hdr->bDescriptorSubType);

	cfg->vs.interface = altsetting->bInterfaceNumber;
	cfg->vs.num_formats = vs_int_hdr->bNumFormats;

	formats = malloc(cfg->vs.num_formats*sizeof(struct format));
	CHECK_ERROR(formats == NULL, -1, "Enable to allocate memory");

	cfg->vs.formats = formats;

	total_len = vs_int_hdr->wTotalLength;
	cur_len = vs_int_hdr->bLength;
	while (cur_len < total_len) {
		vs_int = (const struct uvc_input_header_descriptor *)
				(altsetting->extra + cur_len);
		if(vs_int->bLength == 0) {
			WARNING("Video Streaming descriptor length cannot "
				"be 0. Check the descriptor sizes.");
			break;
		}
		subtype = vs_int->bDescriptorSubType;

		switch(subtype) {
		case VS_FORMAT_MPEG2TS:
			TRACE2("\n\tFound Format MPEG2TS\n");
			break;
		case VS_FORMAT_FRAME_BASED:
			TRACE2("\n\tFound Format FRAME_BASED\n");
			break;
		case VS_FORMAT_UNCOMPRESSED:
			TRACE2("\n\tFound Format UNCOMPRESSED\n");
			break;
		case VS_FORMAT_MJPEG:
			TRACE2("\n\tFound Format MJPEG\n");
			break;
		case VS_FRAME_FRAME_BASED:
			TRACE2("\tFound Frame FRAME_BASED\n");
			break;
		case VS_FRAME_UNCOMPRESSED:
			TRACE2("\tFound Frame UNCOMPRESSED\n");
			break;
		case VS_FRAME_MJPEG:
			TRACE2("\tFound Frame MJPEG\n");
			break;
		case VS_COLORFORMAT:
			TRACE2("\tFound Color Matching descriptor\n");
			break;
		default:
			WARNING("\tUnknown subtype\n");
		}

		switch(subtype) {
		case VS_FORMAT_MPEG2TS:
		case VS_FORMAT_FRAME_BASED:
		case VS_FORMAT_UNCOMPRESSED:
		case VS_FORMAT_MJPEG:
			cur_format++;

			CHECK_ERROR(cur_format >= (int)(cfg->vs.num_formats), -1,
					"Found more formats than specified "
					"by bNumFormats (%i)",
					cfg->vs.num_formats);
			num_frames = 0;
			ret = parse_uvc_format(altsetting->extra + cur_len,
					&formats[cur_format],
					&num_frames);
			if (ret < 0)
				return ret;
			if(num_frames > 0) {
				frames = malloc(formats[cur_format].num_frames *
						sizeof(struct frame));
				CHECK_ERROR(frames == NULL, -1,
						"Enable to allocate memory");
				formats[cur_format].frames = frames;
			} else {
				formats[cur_format].frames = NULL;
			}
			cur_frame = -1;
			break;
		case VS_FRAME_FRAME_BASED:
		case VS_FRAME_UNCOMPRESSED:
		case VS_FRAME_MJPEG:
			cur_frame++;
			CHECK_ERROR(cur_frame >= (int)(formats[cur_format].num_frames),
					-1, "Found more frames (%i) than "
					"specified by bNumFrameDescriptors "
					"(%i)", cur_frame,
					formats[cur_format].num_frames);

			if (frames) {
				ret = parse_uvc_frame(altsetting->extra + cur_len,
					&frames[cur_frame]);
			}
			//if (ret < 0)
			//	return ret;
			break;
		case VS_COLORFORMAT:
			/* Color matching descriptor: ignore it */
			break;
		default:
			WARNING("Unsupported VideoStreaming descriptor: "
				"sub type = %i, length = %i.",
				subtype, vs_int->bLength);
		}
		cur_len += vs_int->bLength;
	}

	/* No endpoint: isoc mode */
	if(altsetting->bNumEndpoints == 0)
		return 0;

	/* Bulk endpoint ? */
	if((altsetting->endpoint->bmAttributes & EP_TYPE_MASK) ==
			EP_TYPE_BULK) {
		CHECK_ERROR(altsetting->bNumEndpoints != 1, -1,
			"Expected only one endpoint in VideoStreaming "
			"interface. Found %i instead",
			altsetting->bNumEndpoints);
		cfg->vs.bulk=1;
		cfg->vs.alt_ep[cfg->vs.num_video_alt] =
				altsetting->endpoint->bEndpointAddress;
		cfg->vs.alt_maxpacketsize[cfg->vs.num_video_alt] =
				altsetting->endpoint->wMaxPacketSize;
		TRACE2("\nVS alt #%i: BULK\n", cfg->vs.num_video_alt);
		TRACE2("VS alt #%i: bEndpointAddress = 0x%x\n",
			cfg->vs.num_video_alt,
			cfg->vs.alt_ep[cfg->vs.num_video_alt]);
		TRACE2("VS alt #%i: wMaxPacketSize = %i\n",
			cfg->vs.num_video_alt,
			cfg->vs.alt_maxpacketsize[cfg->vs.num_video_alt]);
	}

	return 0;
}

static int parse_uvc_format(const unsigned char *desc, struct format *format,
		unsigned int *num_frames)
{
	int subtype, i;
	union {
		const struct uvc_input_header_descriptor *hdr;

		const struct uvc_mp2ts_format_descriptor *format_mp2ts;
		const struct uvc_mjpeg_format_descriptor *format_mjpeg;
		const struct uvc_uncompressed_format_descriptor *format_uncomp;
		const struct uvc_frame_based_format_descriptor *format_frame;
	} vs_int;

	vs_int.hdr = (const struct uvc_input_header_descriptor *) desc;

	subtype = vs_int.hdr->bDescriptorSubType;
	switch(subtype) {
	case VS_FORMAT_MPEG2TS:
		format->type = subtype;
		format->id = vs_int.format_mp2ts->bFormatIndex;
		format->packet_size = vs_int.format_mp2ts->bPacketLength;
		format->num_frames = 0;
		*num_frames = format->num_frames;
		SHOW2(format->id);
		SHOW2(format->packet_size);
		break;
	case VS_FORMAT_FRAME_BASED:
	case VS_FORMAT_UNCOMPRESSED:
	case VS_FORMAT_MJPEG:
		/* FRAME_BASED, UNCOMPRESSED and MJPEG share the
		 * fields we are insterrested in. So just use
		 * vs_int.format_mjpeg to access them */
		format->type = subtype;
		format->id = vs_int.format_mjpeg->bFormatIndex;
		format->num_frames = vs_int.format_mjpeg->bNumFrameDescriptors;
		*num_frames = format->num_frames;
		SHOW2(format->id);
		SHOW2(format->num_frames);
		break;
	default:
		ERROR(-1, "parse_uvc_format called on bDescriptorSubtype = "
				"0x%x\n. parse_uvc_format should only be "
				"called on supported formats\n", subtype);
	}

	/* Get default frame id */
	switch(subtype) {
	case VS_FORMAT_FRAME_BASED:
		format->default_frame_id = vs_int.format_frame->bDefaultFrameIndex;
		SHOW2(format->default_frame_id);
		break;
	case VS_FORMAT_UNCOMPRESSED:
		format->default_frame_id = vs_int.format_uncomp->bDefaultFrameIndex;
		SHOW2(format->default_frame_id);
		break;
	case VS_FORMAT_MJPEG:
		format->default_frame_id = vs_int.format_mjpeg->bDefaultFrameIndex;
		SHOW2(format->default_frame_id);
		break;
	default:
		TRACE2("\n");
		break;
	}

	/* Get format guid: same field for FRAME_BASED and
	 * UNCOMPRESSED format */
	switch(subtype) {
	case VS_FORMAT_FRAME_BASED:
	case VS_FORMAT_UNCOMPRESSED:
		TRACE2("\t\tguid = ");
		for(i=0; i<16; i++) {
			format->guid_format[i] = vs_int.format_frame->guidFormat[i];
			TRACE2("0x%x ", format->guid_format[i]);
		}
		TRACE2("\n");
		break;
	default:
		TRACE2("\n");
		break;
	}

	return 0;
}

static int parse_uvc_frame(const unsigned char *desc, struct frame *frame)
{
	int subtype, i;
	union {
		const struct uvc_input_header_descriptor *hdr;

		const struct uvc_mjpeg_frame_descriptor *frame_mjpeg;
		const struct uvc_uncompressed_frame_descriptor *frame_uncomp;
		const struct uvc_frame_based_frame_descriptor *frame_frame;
	} vs_int;

	vs_int.hdr = (const struct uvc_input_header_descriptor *) desc;

	subtype = vs_int.hdr->bDescriptorSubType;
	switch(subtype) {
	case VS_FRAME_MJPEG:
	case VS_FRAME_UNCOMPRESSED:
		/* UNCOMPRESSED and MJPEG share the
		 * fields we are interested in. So just use
		 * vs_int.format_mjpeg to access them */
		frame->buffer_size = vs_int.frame_mjpeg->dwMaxVideoFrameBufferSize;
		SHOW2(frame->buffer_size);
		frame->type = subtype;
		frame->id = vs_int.frame_mjpeg->bFrameIndex;
		frame->width = vs_int.frame_mjpeg->wWidth;
		frame->height = vs_int.frame_mjpeg->wHeight;
		frame->bitrate_min = vs_int.frame_mjpeg->dwMinBitRate;
		frame->bitrate_max = vs_int.frame_mjpeg->dwMaxBitRate;
		frame->default_fri = vs_int.frame_mjpeg->dwDefaultFrameInterval;
		SHOW2(frame->id);
		SHOW2(frame->width);
		SHOW2(frame->height);
		SHOW2(frame->bitrate_min);
		SHOW2(frame->bitrate_max);
		SHOW2(frame->default_fri);
		if (vs_int.frame_mjpeg->bFrameIntervalType > 0) {
			frame->fri.type = FRI_DISCRETE;
			frame->fri.num = vs_int.frame_mjpeg->bFrameIntervalType;
			CHECK_ERROR(frame->fri.num > vs_int.frame_mjpeg->bFrameIntervalType, -1,
					"Maximum number of Frame Interval "
					"per frame is %i. Got %i",
					vs_int.frame_mjpeg->bFrameIntervalType,
					frame->fri.num);

			frame->fri.discretes = malloc(frame->fri.num *
					sizeof(uint32_t));
			//for (i= 0; i < (int)frame->fri.num && i < 3; i++) {
			for (i= 0; i < (int)frame->fri.num; i++) {
				frame->fri.discretes[i] = vs_int.frame_frame->dwFrameInterval[i];
			}
			SHOW2(frame->fri.type);
			SHOW2(frame->fri.num);
			for (i=0; i<(int)frame->fri.num; i++)
				SHOW2(frame->fri.discretes[i]);
		} else {
			frame->fri.type = FRI_CONTINUOUS;
			frame->fri.min = vs_int.frame_mjpeg->dwFrameInterval[0];
			frame->fri.max = vs_int.frame_mjpeg->dwFrameInterval[1];
			frame->fri.step = vs_int.frame_mjpeg->dwFrameInterval[2];
			SHOW2(frame->fri.type);
			SHOW2(frame->fri.min);
			SHOW2(frame->fri.max);
			SHOW2(frame->fri.step);
			if (frame->fri.min > frame->fri.max) {
				ERROR_NORET("Min Frame Rate Interval (%i) > Max "
						"Frame Rate Interval (%i)",
						frame->fri.min, frame->fri.max);
				/* Workaround buggy firmware: Reverse min and max */
				int tmp;
				tmp = frame->fri.max;
				frame->fri.max = frame->fri.min;
				frame->fri.min = tmp;
				return -1;
			}

			CHECK_ERROR(frame->fri.step > frame->fri.max, -1,
					"Frame Rate Interval step (%i) > Max "
					"Frame Rate Interval (%i)",
					frame->fri.step, frame->fri.max);
		}
		break;
	case VS_FRAME_FRAME_BASED:
		frame->type = subtype;
		frame->id = vs_int.frame_frame->bFrameIndex;
		frame->width = vs_int.frame_frame->wWidth;
		frame->height = vs_int.frame_frame->wHeight;
		frame->bitrate_min = vs_int.frame_frame->dwMinBitRate;
		frame->bitrate_max = vs_int.frame_frame->dwMaxBitRate;
		frame->default_fri = vs_int.frame_frame->dwDefaultFrameInterval;
		SHOW2(frame->id);
		SHOW2(frame->width);
		SHOW2(frame->height);
		SHOW2(frame->bitrate_min);
		SHOW2(frame->bitrate_max);
		SHOW2(frame->default_fri);
		if (vs_int.frame_frame->bFrameIntervalType > 0) {
			frame->fri.type = FRI_DISCRETE;
			frame->fri.num = vs_int.frame_frame->bFrameIntervalType;
			CHECK_ERROR(frame->fri.num > 5, -1,
					"Maximum number of Frame Interval "
					"per frame is 5. Got %i",
					frame->fri.num);

			frame->fri.discretes = malloc(frame->fri.num *
					sizeof(uint32_t));
			memcpy(frame->fri.discretes,
				vs_int.frame_frame->dwFrameInterval,
				frame->fri.num* sizeof(uint32_t));
			SHOW2(frame->fri.type);
			SHOW2(frame->fri.num);
			for (i=0; i<(int)frame->fri.num; i++)
				SHOW2(frame->fri.discretes[i]);
		} else {
			frame->fri.type = FRI_CONTINUOUS;
			frame->fri.min = vs_int.frame_frame->dwFrameInterval[0];
			frame->fri.max = vs_int.frame_frame->dwFrameInterval[1];
			frame->fri.step = vs_int.frame_frame->dwFrameInterval[2];
			SHOW2(frame->fri.type);
			SHOW2(frame->fri.min);
			SHOW2(frame->fri.max);
			SHOW2(frame->fri.step);
			if (frame->fri.min > frame->fri.max) {
				ERROR_NORET("Min Frame Rate Interval (%i) > Max "
						"Frame Rate Interval (%i)",
						frame->fri.min, frame->fri.max);
				/* Workaround buggy firmware: Reverse min and max */
				int tmp;
				tmp = frame->fri.max;
				frame->fri.max = frame->fri.min;
				frame->fri.min = tmp;
				return -1;
			}

			CHECK_ERROR(frame->fri.step > frame->fri.max, -1,
					"Frame Rate Interval step (%i) > Max "
					"Frame Rate Interval (%i)",
					frame->fri.step, frame->fri.max);
		}
		break;
	default:
		ERROR(-1, "parse_uvc_frame called on bDescriptorSubtype = "
				"0x%x\n parse_uvc_frame should only be "
				"called on supported frames type.", subtype);
	}
	return 0;
}

static int parse_uvc_vs_alt(const struct libusb_interface_descriptor *altsetting)
{
	TRACE2("VS alternate setting\n");

	CHECK_ERROR(altsetting->bNumEndpoints != 1, -1,
			"Expected only one endpoint for interface %i alt %i "
			"(got %i)", altsetting->bInterfaceNumber,
			altsetting->bAlternateSetting,
			altsetting->bNumEndpoints);

	cfg->vs.num_video_alt++;
	cfg->vs.alt_ep[cfg->vs.num_video_alt] =
			altsetting->endpoint->bEndpointAddress;
	/* 10..0 specify the maximum packet size (in bytes).
	 * 12..11 specify the number of additional transaction
	 * opportunities per microframe. */
#if 0
	cfg->vs.alt_maxpacketsize[cfg->vs.num_video_alt] =
		(altsetting->endpoint->wMaxPacketSize & 0x7ff) *
		(1 + ((altsetting->endpoint->wMaxPacketSize >> 11) & 0x3));
#else
	cfg->vs.alt_maxpacketsize[cfg->vs.num_video_alt] =
		(altsetting->endpoint->wMaxPacketSize & 0x7ff);
#endif

	TRACE2("VS alt #%i: bEndpointAddress = 0x%x\n", cfg->vs.num_video_alt,
			cfg->vs.alt_ep[cfg->vs.num_video_alt]);
	TRACE2("VS alt #%i: wMaxPacketSize = %i\n", cfg->vs.num_video_alt,
			cfg->vs.alt_maxpacketsize[cfg->vs.num_video_alt]);

	return 0;
}
