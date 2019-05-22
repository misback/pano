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
#include <string.h> /* memset, memcmp */
#include <unistd.h> /* sleep */
#include <assert.h> /* assert */
#include <pthread.h>

#include "common.h"
#include "mxuvc.h"
#include "desc-parser.h"
#include "libusb/handle_events.h"

/******************
 *    UVC defs    *
 ******************/
// VS subtypes
#define VS_UNDEFINED                        0x00
#define VS_INPUT_HEADER                     0x01
#define VS_FORMAT_UNCOMPRESSED              0x04
#define VS_FRAME_UNCOMPRESSED               0x05
#define VS_FORMAT_MJPEG                     0x06
#define VS_FRAME_MJPEG                      0x07
#define VS_FORMAT_MPEG2TS                   0x0a
#define VS_COLORFORMAT                      0x0d
#define VS_FORMAT_FRAME_BASED               0x10
#define VS_FRAME_FRAME_BASED                0x11

// probe requests
#define SET_CUR                             0x01
#define GET_CUR                             0x81
#define GET_MIN                             0x82
#define GET_MAX                             0x83
#define GET_RES                             0x84
#define GET_LEN                             0x85
#define GET_INFO                            0x86
#define GET_DEF                             0x87

// VS interface control selectors
#define VS_PROBE_CONTROL                    0x01
#define VS_COMMIT_CONTROL                   0x02

// PU controls
#define PU_BACKLIGHT_CONTROL                0x01
#define PU_BRIGHTNESS_CONTROL               0x02
#define PU_CONTRAST_CONTROL                 0x03
#define PU_GAIN_CONTROL                     0x04
#define PU_POWER_LINE_FREQUENCY_CONTROL     0x05
#define PU_HUE_CONTROL                      0x06
#define PU_SATURATION_CONTROL               0x07
#define PU_SHARPNESS_CONTROL                0x08
#define PU_GAMMA_CONTROL                    0x09
#define PU_WHITE_BALANCE_TEMP_CONTROL       0x0A
#define PU_WHITE_BALANCE_TEMP_AUTO_CONTROL  0x0B
#define PU_WHITE_BALANCE_COMP_CONTROL       0x0C
#define PU_WHITE_BALANCE_COMP_AUTO_CONTROL  0x0D
#define PU_DIGITAL_MULT_CONTROL             0x0E
#define PU_DIGITAL_MULT_LIMIT_CONTROL       0x0F
#define PU_HUE_AUTO_CONTROL                 0x10
#define PU_ANALOG_VIDEO_STANDard_control    0x11
#define PU_ANALOG_LOCK_STATUS_CONTROL       0x12

//cam control
#define CT_SCANNING_MODE_CONTROL            0x01
#define CT_AE_MODE_CONTROL                  0x02
#define CT_AE_PRIORITY_CONTROL              0x03
#define CT_EXPOSURE_TIME_ABSOLUTE_CONTROL   0x04
#define CT_EXPOSURE_TIME_RELATIVE_CONTROL   0x05
#define CT_FOCUS_ABSOLUTE_CONTROL           0x06
#define CT_FOCUS_RELATIVE_CONTROL           0x07
#define CT_FOCUS_AUTO_CONTROL               0x08

#define CT_IRIS_ABSOLUTE_CONTROL            0x09
#define CT_IRIS_RELATIVE_CONTROL            0x0A
#define CT_ZOOM_ABSOLUTE_CONTROL            0x0B
#define CT_ZOOM_RELATIVE_CONTROL            0x0C
#define CT_PANTILT_ABS_CONTROL              0x0D
#define CT_PANTILT_RELATIVE_CONTROL         0x0E
#define CT_ROLL_ABSOLUTE_CONTROL            0x0F
#define CT_ROLL_RELATIVE_CONTROL            0x10

#define CT_PRIVACY_CONTROL                  0x11

/* Controls in the XU */
enum AVC_XU_CTRL {
	AVC_XU_PROFILE = 1,
	AVC_XU_LEVEL,
	AVC_XU_PICTURE_CODING,
	AVC_XU_RESOLUTION,
	AVC_XU_GOP_STRUCTURE,
	AVC_XU_GOP_LENGTH,
	AVC_XU_BITRATE,
	AVC_XU_FORCE_I_FRAME,
	AVC_XU_MAX_NAL,
	AVC_XU_VUI_ENABLE,
	AVC_XU_PIC_TIMING_ENABLE,
	AVC_XU_GOP_HIERARCHY_LEVEL,
	AVC_XU_AV_MUX_ENABLE,
	AVC_XU_MAX_FRAME_SIZE,
	AVC_XU_FIRST_IFRAME_QP,
	AVC_XU_GOP_HIERARCHY_QPDELTA_0,
	AVC_XU_GOP_HIERARCHY_QPDELTA_1,
	AVC_XU_GOP_HIERARCHY_QPDELTA_2,
	AVC_XU_GOP_HIERARCHY_QPDELTA_3,
	AVC_XU_GOP_HIERARCHY_QPDELTA_4,
	AVC_XU_GOP_HIERARCHY_QPDELTA_5,
	AVC_XU_GOP_HIERARCHY_QPDELTA_6,
	AVC_XU_GOP_HIERARCHY_QPDELTA_7,
	AVC_XU_LONG_TERM_REF_INTERVAL_0,
	AVC_XU_LONG_TERM_REF_INTERVAL_1,
	AVC_XU_JPEG_COMPRESSION_QUALITY,
	AVC_XU_START_SKYPE_BULK_CHANNEL,
	AVC_XU_NUM_CTRLS = AVC_XU_START_SKYPE_BULK_CHANNEL,
};

/* Controls in the XU */
enum PU_XU_CTRL {
    PU_XU_ANF_ENABLE = 1,
    PU_XU_NF_STRENGTH,
    PU_XU_TF_STRENGTH,
    PU_XU_SINTER,
    PU_XU_ADAPTIVE_WDR_ENABLE,
    PU_XU_WDR_STRENGTH,
    PU_XU_SENSOR_GAIN,
    PU_XU_EXPOSURE_TIME,
    PU_XU_AWB_PARAMS,
    PU_XU_WB_TEMPERATURE,
    PU_XU_VFLIP,
    PU_XU_HFLIP,
    PU_XU_WB_ZONE_SEL_ENABLE,
    PU_XU_WB_ZONE_SEL,
    PU_XU_EXP_ZONE_SEL_ENABLE,
    PU_XU_EXP_ZONE_SEL,
    PU_XU_MAX_ANALOG_GAIN,
    PU_XU_HISTO_EQ,
    PU_XU_SHARPEN_FILTER,
    PU_XU_GAIN_MULTIPLIER,
    PU_XU_CROP_MODE,
    PU_XU_EXP_MIN_FR_RATE,
    PU_XU_DEWARP_PARAMS1,
    PU_XU_DEWARP_PARAMS2,
    PU_XU_COMPOSITOR_PARAMS,
    PU_XU_CONFIG_PARAMS,
    PU_XU_SATURATION_MODE,
    PU_XU_BRIGHTNESS_MODE,
    PU_XU_CONTRAST_MODE,
    PU_XU_MVMT_QUERY,
    PU_XU_SENSOR_FRAMERATE,
    PU_XU_AEROI,
    PU_XU_NUM_CTRLS = PU_XU_AEROI
};


/* Semantics of AVC_XU_PICTURE_CODING control values */
enum PICTURE_CODING {
	PICTURE_CODING_FRAME,
	PICTURE_CODING_FIELD,
	PICTURE_CODING_MBAFF,
};

/* Semantics of AVC_XU_GOP_STRUCTURE control values */
enum GOP_STRUCTURE {
	GOP_STRUCTURE_IP,
	GOP_STRUCTURE_IBP,
	GOP_STRUCTURE_IBBP,
	GOP_STRUCTURE_IBBRBP
};

/* Semantics of AVC_XU_FORCE_I_FRAME control values */
enum FORCE_I_FRAME {
	FORCE_I_FRAME = 1,
};

struct uvc_stream_params {
	uint16_t bmHint;
	uint8_t bFormatIndex;
	uint8_t bFrameIndex;
	uint32_t dwFrameInterval;
	uint16_t wKeyFrameRate;
	uint16_t wPFrameRate;
	uint16_t wCompQuality;
	uint16_t wCompWindowSize;
	uint16_t wDelay;
	uint32_t dwMaxVideoFrameSize;
	uint32_t dwMaxPayloadTransferSize;
	uint32_t dwClockFrequency;
	uint8_t bmFramingInfo;
	uint8_t bPreferedVersion;
	uint8_t bMinVersion;
	uint8_t bMaxVersion;
} PACKED;

/* Control request type */
#define REQ_SET         0x21
#define REQ_GET         0xa1

#define PACKETS_PER_TRANSFER_DEFAULT 270
#define NUM_TRANSFERS_DEFAULT 4
#define CHECK_H264_CONTINUITY_DEFAULT 0
/* isoc transfer over libusb has limitations */
#define MAX_PACKETS_PER_ISOC_TRANSFER 125

/*****************************
 *    Internal structures    *
 *****************************/
typedef enum ctrl_id {
	CTRL_BRIGHTNESS                 = 1,
	CTRL_CONTRAST                   = 2,
	CTRL_GAIN                       = 3,
	CTRL_HUE                        = 4,
	CTRL_SATURATION                 = 5,
	CTRL_SHARPNESS                  = 6,
	CTRL_GAMMA                      = 7,
	CTRL_DIGMULT                    = 8,
	CTRL_PANTILT_ABS                = 9,
	CTRL_PROFILE                    = 10,
	CTRL_LEVEL                      = 11,
	CTRL_PICTCODING                 = 12,
	CTRL_RESOLUTION                 = 13,
	CTRL_GOPSTRUCT                  = 14,
	CTRL_GOPLEN                     = 15,
	CTRL_BITRATE                    = 16,
	CTRL_IFRAME                     = 17,
	CTRL_BACKLIGHT                  = 18,
	CTRL_MAXNAL                     = 19,
	CTRL_VFLIP                      = 20,
	CTRL_HFLIP                      = 21,
	CTRL_ADAPTIVE_WDR_ENABLE        = 22,
	CTRL_WDR_STRENGTH               = 23,
	CTRL_AVMUX                      = 24,
	CTRL_FRAMESIZE                  = 25,
	CTRL_MAX_ANALOG_GAIN            = 26,
	CTRL_HISTO_EQ                   = 27,
	CTRL_SHARPEN_FILTER             = 28,
	CTRL_GAIN_MULTIPLIER            = 29,
	CTRL_SENSOR_FRAMERATE	= 30,
	CTRL_START_SKYPE_BULK_CHANNEL   = 31,
	NUM_CTRL                        = CTRL_START_SKYPE_BULK_CHANNEL
} ctrl_id_t;

/* Part of a control that is fixed */
struct ctrl_cfg {
	int type;
	int cs;
	int bit_bmcontrols;
	int len;
};
/* Part of a control that can changed depending on the channel */
struct ctrl_settings {
	int unit;
	int64_t min;
	int64_t max;
	int enabled;
};
typedef struct {
	ctrl_id_t id;
	struct ctrl_cfg cfg;
	struct ctrl_settings settings[NUM_VID_CHANNEL];
} VIDEO_CTRL;

#define PROC     0
#define CAM      1
#define AVCEXT   2
#define PUEXT    3
VIDEO_CTRL uvc_controls[] = {
{CTRL_BRIGHTNESS,              {PROC,   PU_BRIGHTNESS_CONTROL,       0,                             2}, {}},
{CTRL_CONTRAST,                {PROC,   PU_CONTRAST_CONTROL,         1,                             2}, {}},
{CTRL_HUE,                     {PROC,   PU_HUE_CONTROL,              2,                             2}, {}},
{CTRL_SATURATION,              {PROC,   PU_SATURATION_CONTROL,       3,                             2}, {}},
{CTRL_SHARPNESS,               {PROC,   PU_SHARPNESS_CONTROL,        4,                             2}, {}},
{CTRL_GAMMA,                   {PROC,   PU_GAMMA_CONTROL,            5,                             2}, {}},
{CTRL_BACKLIGHT,               {PROC,   PU_BACKLIGHT_CONTROL,        8,                             4}, {}},
{CTRL_GAIN,                    {PROC,   PU_GAIN_CONTROL,             9,                             2}, {}},
{CTRL_DIGMULT,                 {PROC,   PU_DIGITAL_MULT_CONTROL,     14,                            2}, {}},
{CTRL_PANTILT_ABS,             {CAM,    CT_PANTILT_ABS_CONTROL,      11,                            8}, {}},
{CTRL_PROFILE,                 {AVCEXT, AVC_XU_PROFILE,              AVC_XU_PROFILE-1,              4}, {}},
{CTRL_LEVEL,                   {AVCEXT, AVC_XU_LEVEL,                AVC_XU_LEVEL-1,                4}, {}},
{CTRL_PICTCODING,              {AVCEXT, AVC_XU_PICTURE_CODING,       AVC_XU_PICTURE_CODING-1,       4}, {}},
{CTRL_RESOLUTION,              {AVCEXT, AVC_XU_RESOLUTION,           AVC_XU_RESOLUTION-1,           4}, {}},
{CTRL_GOPSTRUCT,               {AVCEXT, AVC_XU_GOP_STRUCTURE,        AVC_XU_GOP_STRUCTURE-1,        4}, {}},
{CTRL_GOPLEN,                  {AVCEXT, AVC_XU_GOP_LENGTH,           AVC_XU_GOP_LENGTH-1,           4}, {}},
{CTRL_BITRATE,                 {AVCEXT, AVC_XU_BITRATE,              AVC_XU_BITRATE-1,              4}, {}},
{CTRL_IFRAME,                  {AVCEXT, AVC_XU_FORCE_I_FRAME,        AVC_XU_FORCE_I_FRAME-1,        4}, {}},
{CTRL_MAXNAL,                  {AVCEXT, AVC_XU_MAX_NAL,              AVC_XU_MAX_NAL-1,              4}, {}},
{CTRL_AVMUX,                   {AVCEXT, AVC_XU_AV_MUX_ENABLE,        AVC_XU_AV_MUX_ENABLE-1,        4}, {}},
{CTRL_FRAMESIZE,               {AVCEXT, AVC_XU_MAX_FRAME_SIZE,       AVC_XU_MAX_FRAME_SIZE-1,       4}, {}},
{CTRL_VFLIP,                   {PUEXT,  PU_XU_VFLIP,                 PU_XU_VFLIP-1,                 4}, {}},
{CTRL_HFLIP,                   {PUEXT,  PU_XU_HFLIP,                 PU_XU_HFLIP-1,                 4}, {}},
{CTRL_ADAPTIVE_WDR_ENABLE,     {PUEXT,  PU_XU_ADAPTIVE_WDR_ENABLE,   PU_XU_ADAPTIVE_WDR_ENABLE-1,   4}, {}},
{CTRL_WDR_STRENGTH,            {PUEXT,  PU_XU_WDR_STRENGTH,          PU_XU_WDR_STRENGTH-1,          4}, {}},
{CTRL_MAX_ANALOG_GAIN,         {PUEXT,  PU_XU_MAX_ANALOG_GAIN,       PU_XU_MAX_ANALOG_GAIN-1,       4}, {}},
{CTRL_HISTO_EQ,                {PUEXT,  PU_XU_HISTO_EQ,              PU_XU_HISTO_EQ-1,              4}, {}},
{CTRL_SHARPEN_FILTER,          {PUEXT,  PU_XU_SHARPEN_FILTER,        PU_XU_SHARPEN_FILTER-1,        4}, {}},
{CTRL_GAIN_MULTIPLIER,         {PUEXT,  PU_XU_GAIN_MULTIPLIER,       PU_XU_GAIN_MULTIPLIER-1,       4}, {}},
{CTRL_SENSOR_FRAMERATE,        {PUEXT,	PU_XU_SENSOR_FRAMERATE,	     PU_XU_SENSOR_FRAMERATE-1,		4}, {}},
{CTRL_START_SKYPE_BULK_CHANNEL,{AVCEXT, AVC_XU_START_SKYPE_BULK_CHANNEL, AVC_XU_START_SKYPE_BULK_CHANNEL-1, 4}, {}},
{0, {0, 0, 0, 0}, {}}
};


struct cb_user_data {
	video_channel_t ch;
	unsigned int xfer_idx;
};

/* GUIDs */
static uint8_t guid_raw_h264[16] = {
		0x48, 0x32, 0x36, 0x34, 0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71};
static uint8_t guid_yuy2[16] = {
		0x59, 0x55, 0x59, 0x32, 0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71};
static uint8_t guid_nv12[16] = {
		0x4e, 0x56, 0x31, 0x32, 0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71};
static uint8_t guid_avcext[16] = {
		0xd9, 0x92, 0x2b, 0xba, 0xf2, 0x26, 0x94, 0x42,
		0x42, 0xae, 0xe4, 0xeb, 0x4d, 0x68, 0xdd, 0x06};
static uint8_t guid_puext[16] = {
		0x12, 0xcd, 0x5d, 0xdf, 0x5f, 0x7d, 0xba, 0x4b,
		0xbb, 0x6d, 0x4b, 0x62, 0x5a, 0xdd, 0x52, 0x72};

static struct libusb_device_handle *video_hdl;
static struct libusb_context       *video_ctx;
static volatile int video_disconnected = 1;
static volatile int video_initialized = 0;
static int active_transfers[NUM_VID_CHANNEL];

static unsigned int packets_per_transfer;
static unsigned int num_transfers;
static unsigned int check_h264_continuity;

static unsigned char *gFrameBuf1[NUM_VID_CHANNEL] = {NULL};
static unsigned char *gFrameBuf2[NUM_VID_CHANNEL] = {NULL};

int ipcam_mode;


#if DEBUG_VIDEO_CALLBACK > 0
static int dead_counter[NUM_VID_CHANNEL];
#endif

extern struct video_cfg            mxuvc_vcfg[NUM_VID_CHANNEL];
extern struct video_settings       mxuvc_vset[NUM_VID_CHANNEL];

/*******************
 *    Functions    *
 *******************/
static int set_pantilt(video_channel_t ch, int32_t *pan, int32_t *tilt);
static int get_pantilt(video_channel_t ch, int32_t *pan, int32_t *tilt);

static inline void incr_active_transfers(video_channel_t ch) {
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&mutex);
	active_transfers[ch]++;
	pthread_mutex_unlock(&mutex);
}
static inline void decr_active_transfers(video_channel_t ch) {
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&mutex);
	active_transfers[ch]--;
	pthread_mutex_unlock(&mutex);
}
static inline int get_active_transfers(video_channel_t ch) {
	int val;
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&mutex);
	val = active_transfers[ch];
	pthread_mutex_unlock(&mutex);
	return val;
}
static inline int set_active_transfers(video_channel_t ch, int val) {
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&mutex);
	active_transfers[ch] = val;
	pthread_mutex_unlock(&mutex);
	return val;
}

static inline struct video_cfg* get_vcfg(video_channel_t ch)
{
	return &mxuvc_vcfg[ch];
}
static inline struct video_settings* get_vset(video_channel_t ch)
{
	return &mxuvc_vset[ch];
}

static const char* uvcformat2str(int format)
{
	switch (format) {
	case VS_FORMAT_MPEG2TS:
		return "VS_FORMAT_MPEG2TS";
	case VS_FORMAT_FRAME_BASED:
		return "VS_FORMAT_FRAME_BASED";
	case VS_FORMAT_MJPEG:
		return "VS_FORMAT_MJPEG";
	case VS_FORMAT_UNCOMPRESSED:
		return "VS_FORMAT_UNCOMPRESSED";
	default:
		return "Unknown";
	}
}
static const char* ctrl2str(ctrl_id_t ctrl)
{
	static const char *mapping[NUM_CTRL+1] = {
	CST2STR(CTRL_BRIGHTNESS),
	CST2STR(CTRL_CONTRAST),
	CST2STR(CTRL_GAIN),
	CST2STR(CTRL_HUE),
	CST2STR(CTRL_SATURATION),
	CST2STR(CTRL_SHARPNESS),
	CST2STR(CTRL_GAMMA),
	CST2STR(CTRL_DIGMULT),
	CST2STR(CTRL_PANTILT_ABS),
	CST2STR(CTRL_PROFILE),
	CST2STR(CTRL_LEVEL),
	CST2STR(CTRL_PICTCODING),
	CST2STR(CTRL_RESOLUTION),
	CST2STR(CTRL_GOPSTRUCT),
	CST2STR(CTRL_GOPLEN),
	CST2STR(CTRL_BITRATE),
	CST2STR(CTRL_IFRAME),
	CST2STR(CTRL_BACKLIGHT),
	CST2STR(CTRL_MAXNAL),
	CST2STR(CTRL_AVMUX),
	CST2STR(CTRL_FRAMESIZE),
	CST2STR(CTRL_VFLIP),
	CST2STR(CTRL_HFLIP),
	CST2STR(CTRL_ADAPTIVE_WDR_ENABLE),
	CST2STR(CTRL_WDR_STRENGTH),
	CST2STR(CTRL_MAX_ANALOG_GAIN),
	CST2STR(CTRL_HISTO_EQ),
	CST2STR(CTRL_SHARPEN_FILTER),
	CST2STR(CTRL_GAIN_MULTIPLIER),
	CST2STR(CTRL_SENSOR_FRAMERATE),
	CST2STR(CTRL_START_SKYPE_BULK_CHANNEL)
	};
	return mapping[ctrl];
}

static VIDEO_CTRL* get_ctrl_by_id(ctrl_id_t id)
{
	VIDEO_CTRL *control = uvc_controls;
	while(control->id) {
		if(control->id == id)
			return control;
		control++;
	}

	ERROR(NULL, "Unexpected error: no match for ctrl_id %i in "
			"get_ctrl_by_id()\n", id);
}

static int get_ctrl(video_channel_t ch, ctrl_id_t id, int get_type, void *val)
{
	int ret;
	VIDEO_CTRL *ctrl = NULL;
	uint8_t	bmRequestType = REQ_GET;
	uint8_t bRequest = get_type;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	unsigned char *data = (unsigned char *) val;
	struct video_cfg *vcfg;

	ctrl = get_ctrl_by_id(id);
	if(ctrl == NULL)
		return -1;

	wValue = ctrl->cfg.cs << 8;

	if (get_type == GET_LEN)
		wLength = 2;
	else
		wLength = ctrl->cfg.len;

	vcfg = get_vcfg(ch);
	CHECK_ERROR(vcfg->enabled == 0, -1,
			"Unable to get %s. %s channel is not "
			"enabled.", ctrl2str(id), chan2str(ch));
	CHECK_ERROR(ctrl->settings[ch].enabled == 0, -1,
			"%s control is not enabled on %s channel.",
			ctrl2str(id), chan2str(ch));

	wIndex = vcfg->vc.interface + (ctrl->settings[ch].unit << 8);

	ret = libusb_control_transfer(video_hdl,
			bmRequestType,
			bRequest,
			wValue,
			wIndex,
			data,
			wLength,
			USB_TIMEOUT);

	return ret;
}

static int set_ctrl(video_channel_t ch, ctrl_id_t id, int set_type, void *val)
{
	int ret;
	VIDEO_CTRL *ctrl = NULL;
	uint8_t	bmRequestType = REQ_SET;
	uint8_t bRequest = set_type;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	unsigned char *data = (unsigned char *) val;
	struct video_cfg *vcfg;

	ctrl = get_ctrl_by_id(id);
	if(ctrl == NULL)
		return -1;

	wValue = ctrl->cfg.cs << 8;
	wLength = ctrl->cfg.len;

	vcfg = get_vcfg(ch);
	CHECK_ERROR(vcfg->enabled == 0, -1,
			"Unable to set %s. %s channel is not "
			"enabled.", ctrl2str(id), chan2str(ch));
	CHECK_ERROR(ctrl->settings[ch].enabled == 0, -1,
			"%s control is not enabled on %s channel.",
			ctrl2str(id), chan2str(ch));

	wIndex = vcfg->vc.interface + (ctrl->settings[ch].unit << 8);

	ret = libusb_control_transfer(video_hdl,
			bmRequestType,
			bRequest,
			wValue,
			wIndex,
			data,
			wLength,
			USB_TIMEOUT);

	return ret;
}

static int probe_strm_params(video_channel_t ch,
		struct uvc_stream_params *params, int req_type)
{
	int ret;

	uint8_t	bmRequestType = REQ_SET;
	uint8_t bRequest = req_type;
	uint16_t wValue = VS_PROBE_CONTROL << 8;
	uint16_t wIndex;
	uint16_t wLength = 34;
	unsigned char *data = (unsigned char *) params;
	struct video_cfg *vcfg;

	vcfg = get_vcfg(ch);
	CHECK_ERROR(vcfg->enabled == 0, -1,
			"Unable to probe stream params. %s channel is not "
			"enabled", chan2str(ch));

	if(req_type == SET_CUR)
		bmRequestType = REQ_SET;
	else
		bmRequestType = REQ_GET;

	wIndex = vcfg->vs.interface;

	TRACE2("Probing stream params (%s) for %s channel on interface #%i\n",
			bmRequestType == REQ_SET ? "SET" : "GET",
			chan2str(ch), vcfg->vs.interface);

	ret = libusb_control_transfer(video_hdl,
			bmRequestType,
			bRequest,
			wValue,
			wIndex,
			data,
			wLength,
			USB_TIMEOUT);

	return ret;
}

static int commit_strm_params(video_channel_t ch,
		struct uvc_stream_params *params, int req_type)
{
	int ret;

	uint8_t	bmRequestType = REQ_SET;
	uint8_t bRequest = req_type;
	uint16_t wValue = VS_COMMIT_CONTROL << 8;
	uint16_t wIndex;
	uint16_t wLength = 34;
	unsigned char *data = (unsigned char *) params;
	struct video_cfg *vcfg;

	vcfg = get_vcfg(ch);
	CHECK_ERROR(vcfg->enabled == 0, -1,
			"Unable to commit stream params. %s channel is not "
			"enabled", chan2str(ch));

	if(req_type == SET_CUR)
		bmRequestType = REQ_SET;
	else
		bmRequestType = REQ_GET;

	wIndex = vcfg->vs.interface;

	TRACE2("Committing stream params (%s) for %s channel on interface "
			"#%i\n", bmRequestType == REQ_SET ? "SET" : "GET",
			chan2str(ch), vcfg->vs.interface);

	ret = libusb_control_transfer(video_hdl,
			bmRequestType,
			bRequest,
			wValue,
			wIndex,
			data,
			wLength,
			USB_TIMEOUT);

	return ret;
}

/* TODO: allocate frame size dynamically */
#define MAX_FRAME_SIZE 530*1024 /* should be at least 530 kB */
static void demux_uvc(video_channel_t ch, unsigned char *buf, int len)
{
	static int last_fid[NUM_VID_CHANNEL] = {0};
	static int last_eof[NUM_VID_CHANNEL] =
			{[0 ... (NUM_VID_CHANNEL-1)] = 1};
	static int frame_len[NUM_VID_CHANNEL] = {0};
	static unsigned char framebuf0[NUM_VID_CHANNEL][MAX_FRAME_SIZE];
	static unsigned char framebuf1[NUM_VID_CHANNEL][MAX_FRAME_SIZE];
	static unsigned char *framebuf[NUM_VID_CHANNEL];
	static int frame_error[NUM_VID_CHANNEL];
	static long long lastbytepos[NUM_VID_CHANNEL] = {0};
	static long long bytepos[NUM_VID_CHANNEL] = {0};
	static int lastcc[NUM_VID_CHANNEL]={[0 ... (NUM_VID_CHANNEL-1)] = -1};
	static uint32_t ts[NUM_VID_CHANNEL]={[0 ... (NUM_VID_CHANNEL-1)] = 0};
	int hdr_len, fid, eof;
	struct video_settings *vset;
	struct video_cfg *vcfg;

	vset = get_vset(ch);
	vcfg = get_vcfg(ch);

	/* Check if video just started. Reset static values in that case */
	if(vset->first_transfer) {
		vset->first_transfer = 0;
		last_fid[ch] = 0;
		last_eof[ch] = 1;
		frame_len[ch] = 0;
		frame_error[ch] = 0;
		lastbytepos[ch] = 0;
		bytepos[ch] = 0;
		lastcc[ch] = -1;
		framebuf[ch] = framebuf0[ch];
	}

	hdr_len = buf[0];
	fid = buf[1] & 0x1;
	eof = (buf[1]>>1) & 0x1;
	if (len <= hdr_len || hdr_len < 2 || hdr_len > 12) {
		TRACE2("Invalid UVC header size on %s channel after "
			"frame #%i: %i bytes. Skipping.\n",
			chan2str(ch), vset->frame_count, hdr_len);
		return;
	}

	if (last_fid[ch] != fid) {
		/* Warn about Packet loss */
		if (last_eof[ch] != 1) {
			WARNING("Packet loss on %s channel after frame "
				"#%i: got new frame id before the end "
				"of the current frame.\n", chan2str(ch), vset->frame_count);
			SHOW(len);
			SHOW(hdr_len);
			SHOW(frame_len[ch]);
			frame_error[ch] = 1;
		}
		last_fid[ch] = fid;
		frame_len[ch] = 0;
		if(!vcfg->vs.bulk) {
			if(framebuf[ch] == framebuf0[ch])
				framebuf[ch] = framebuf1[ch];
			else
				framebuf[ch] = framebuf0[ch];
		}
	}


	last_eof[ch] = eof;

	buf += hdr_len;
	len -= hdr_len;

	/* Demux TS */
	/*  - If format is set to RAW_H264 but is not supported by the camera,
	 *    capture MPEG2TS and demux here.
	 *  - If check_h264_continuity is set to 1, capture MPEG2TS, use its
	 *    header for continuity check and then demux before passing it
	 *    to the user callback.*/
	if(vset->quirk_demuxts ||
			vset->cur_video_format == VID_FORMAT_H264_TS ||
			vset->cur_video_format == VID_FORMAT_H264_AAC_TS) {
		int i;

		for(i = 0; i < len; i+= 188)
		{
			const unsigned char *p = buf + i;
			int start = p[1] & 0x40;
			int pid = (((int)p[1] & 0x1f) << 8) | p[2];

			// FIXME - hardcoded PID
			if( pid != 0x1011 )
				continue;

			int af = p[3] & 0x20;
			int pl = p[3] & 0x10;
			int cc = p[3] & 0x0f;
			if ((lastcc[ch] != -1) && (pl == 0x10) &&
				(((lastcc[ch]+1) & 0xf) != cc) &&
				frame_error[ch] == 0)
			{
				TRACE("** MPEG2-TS continuity error on %s channel ",
						chan2str(ch));
				TRACE("(expected %d vs %d) at byte %lld\n",
						(lastcc[ch]+1) & 0xf, cc,
						bytepos[ch]+i);
				TRACE("   (%lld bytes/%lld packets from last error)\n",
						bytepos[ch]+i-lastbytepos[ch],
						(bytepos[ch]+i-lastbytepos[ch])/188);
				frame_error[ch] = 1;
			}
			lastcc[ch] = cc;
			lastbytepos[ch] = bytepos[ch];

			if(frame_error[ch])
				continue;

			int ps = 184;
			if(!pl)
				continue;
			p += 4;

			if(af) {
				ps -= p[0] + 1;
				if (ps < 0 || ps > 184) {
					WARNING("ps < 0 || ps > 184: ps = %i\n", ps);
					frame_error[ch] = 1;
					continue;
				}
				p += p[0] + 1;
			}

			// PES is here
			if(start) {
				if(frame_len[ch] && vset->quirk_demuxts) {
					WARNING("Full NAL unit detected "
						"before end of UVC frame. Skipping\n");
					frame_len[ch] = 0;
				}
				uint64_t pts=0;
				int j, pes_len;

				for (j = 6; j < 22; j++)
					if (0xff != p[j])
						break;

				if ((p[j] & 0xC0)>>6 == 0x02)
				{
					pts  = (long long)((p[j+3] & 0x0E)>>1)<<30;
					pts |= (long long)((p[j+4]<<8 | p[j+5])>>1)<<15;
					pts |= (long long)((p[j+6]<<8 | p[j+7])>>1);
				}

				ts[ch] = (uint32_t) (pts & 0xffffffff);

				pes_len = j + 2 + p[j+2] + 1;
				ps -= pes_len;
				p += pes_len;
			}

			if(vset->quirk_demuxts) {
				if(frame_len[ch] + ps >= MAX_FRAME_SIZE)
					continue;
				memcpy(framebuf[ch] + frame_len[ch], p, ps);
				frame_len[ch] += ps;
			}
		}
		bytepos[ch] += len;
	}

	if(!vset->quirk_demuxts) {
		if(vcfg->vs.bulk) {
			/* For Bulk we just move the pointer after the header */
			framebuf[ch] = buf;
			frame_len[ch] = len;
		} else {
			/* For ISOC we have to memcpy */
			assert (frame_len[ch] + len <= MAX_FRAME_SIZE);
			memcpy(framebuf[ch] + frame_len[ch], buf, len);
			frame_len[ch] += len;
		}
	}

	if(eof) {
		if(frame_error[ch]) {
			frame_error[ch] = 0;
			frame_len[ch] = 0;
			ERROR_NORET("Error detected in frame. Skipping");
			return;
		}
		vset->frame_count++;
		if(vset->cb == NULL)
			WARNING("Video callback function not "
				"registered for %s channel.",
				chan2str(ch));
		else {
			video_info_t info;
			/* If not a transport stream then create a timestamp here
			 * with gettimeofday */
			if (!vset->quirk_demuxts &&
				vset->cur_video_format != VID_FORMAT_H264_TS &&
				vset->cur_video_format != VID_FORMAT_H264_AAC_TS) {
				struct timeval tv;
				uint64_t ts64 = 0, tsec = 0, tusec = 0;
				gettimeofday(&tv, NULL);
				tsec = (uint64_t) tv.tv_sec;
				tusec = (uint64_t) tv.tv_usec;
				ts64 = (tsec*1000000 + tusec)*9/100;
				ts[ch] = (uint32_t) (ts64 & 0xffffffff);
				info.ts = ts[ch];
			}

			/* Run the callback function registered for the channel */
			info.format = vset->cur_video_format;
			info.stats.buf = NULL;
			info.stats.size = 0;
			vset->cb(framebuf[ch], frame_len[ch],
					info,
					vset->cb_user_data);
		}
		frame_len[ch] = 0;
	}
}

static void free_video_transfer(struct libusb_transfer **transfers,
		unsigned id)
{
	if(transfers == NULL || transfers[id] == NULL)
		return;

	free(transfers[id]->buffer);
	free((struct cb_user_data*) transfers[id]->user_data);
	libusb_free_transfer(transfers[id]);
	transfers[id] = NULL;
}

static void capture_callback(struct libusb_transfer *transfer)
{
	struct cb_user_data *udata;
	video_channel_t ch;
	struct video_settings *vset;
	struct video_cfg *vcfg;

	assert(transfer != NULL);

	udata = (struct cb_user_data*) transfer->user_data;
	ch = udata->ch;

	decr_active_transfers(ch);

	vset = get_vset(ch);
	vcfg = get_vcfg(ch);

	switch(transfer->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		break;
	case LIBUSB_TRANSFER_CANCELLED:
		free_video_transfer(vset->xfers, udata->xfer_idx);
		TRACE2("Transfer canceled on %s channel\n", chan2str(ch));
		return;
	case LIBUSB_TRANSFER_NO_DEVICE:
		free_video_transfer(vset->xfers, udata->xfer_idx);
		if(!video_disconnected) {
			ERROR_NORET("The camera has been disconnected.");
			video_disconnected = 1;
		}
		return;
	case LIBUSB_TRANSFER_ERROR:
		free_video_transfer(vset->xfers, udata->xfer_idx);
		if(!video_disconnected) {
			ERROR_NORET("The USB transfer did not complete on %s channel.",
					chan2str(ch));
			video_disconnected = 1;
		}
		return;
	case LIBUSB_TRANSFER_TIMED_OUT:
		ERROR_NORET("The USB transfer timed-out on %s channel.",
				chan2str(ch));
		if(vset->started && libusb_submit_transfer(transfer) >= 0)
			incr_active_transfers(ch);
		else
			free_video_transfer(vset->xfers, udata->xfer_idx);
		return;
	case LIBUSB_TRANSFER_STALL:
		ERROR_NORET("The USB transfer stalled on %s channel.",
				chan2str(ch));
		if(vset->started && libusb_submit_transfer(transfer) >= 0)
			incr_active_transfers(ch);
		else
			free_video_transfer(vset->xfers, udata->xfer_idx);
		return;
	case LIBUSB_TRANSFER_OVERFLOW:
		ERROR_NORET("The USB transfer overflowed on %s channel.",
				chan2str(ch));
		if(vset->started && libusb_submit_transfer(transfer) >= 0)
			incr_active_transfers(ch);
		else
			free_video_transfer(vset->xfers, udata->xfer_idx);
		return;
	default:
		ERROR_NORET("Unknown video USB transfer return status on %s "
				"channel.", chan2str(ch));
		if(vset->started && libusb_submit_transfer(transfer) >= 0)
			incr_active_transfers(ch);
		else
			free_video_transfer(vset->xfers, udata->xfer_idx);
		return;
	}

	/* Do nothing if the channel has been stopped */
	if (vset->started == 0) {
		free_video_transfer(vset->xfers, udata->xfer_idx);
		return;
	}


#if DEBUG_VIDEO_CALLBACK > 0
	/* Get time for further monitoring */
	struct timeval tv_start, tv_end;
	static struct timeval tv_start_prev[NUM_VID_CHANNEL];
	struct timezone tz;
	int delta_time;
	static int max_delta_time = 0, time_counter = 0;
	gettimeofday(&tv_start,&tz);

	delta_time = (1000000*tv_start.tv_sec + tv_start.tv_usec) -
		(1000000*tv_start_prev[ch].tv_sec + tv_start_prev[ch].tv_usec);
	TRACE("Frame delta time %s: %i ms\n", chan2str(ch), delta_time/1000);
	tv_start_prev[ch] = tv_start;

#endif
	/* If Bulk endpoint */
	if (vcfg->vs.bulk) {
		static unsigned char *framebuf[NUM_VID_CHANNEL];
		static unsigned int framelen[NUM_VID_CHANNEL];

		if(vset->first_transfer) {
			framelen[ch] = 0;
			framebuf[ch] = gFrameBuf1[ch];
		}

		framelen[ch] += transfer->actual_length;
		if(framelen[ch] <= vset->max_frame_size) {
			memcpy(framebuf[ch] + framelen[ch] - transfer->actual_length,
					transfer->buffer,
					transfer->actual_length);
		} else {
			WARNING("Too big! Length %d exceeding max_frame_size %d for channel %d",framelen[ch],vset->max_frame_size,ch);
			framelen[ch] = 0;
		}

		assert(framelen[ch] <= vset->max_frame_size);
		if(transfer->actual_length != transfer->length ||
				framelen[ch] == vset->max_frame_size) {
			demux_uvc(ch, framebuf[ch], framelen[ch]);
			if(framebuf[ch] == gFrameBuf1[ch])
				framebuf[ch] = gFrameBuf2[ch];
			else
				framebuf[ch] = gFrameBuf1[ch];
			framelen[ch] = 0;
		}
	} else {
		/* If Isoc endpoint */
		//struct libusb_iso_packet_descriptor *desc;
		unsigned char *buf;
		unsigned int j;
		for(j=0; j<packets_per_transfer; j++) {
			if (transfer->iso_packet_desc[j].actual_length > 0) {
				//buf = libusb_get_iso_packet_buffer_simple(transfer, j);
				buf = transfer->buffer +
					(transfer->iso_packet_desc[0].length * j);
				demux_uvc(ch, buf,
					transfer->iso_packet_desc[j].actual_length);
#if DEBUG_VIDEO_CALLBACK > 0
				dead_counter[ch] = 0;
#endif
			}
#if DEBUG_VIDEO_CALLBACK > 0
			else {
				dead_counter[ch]++;
				/* 40000 * 125us = 5 seconds */
				if (dead_counter[ch] <= 40000)
					continue;

				dead_counter[ch] = 0;
				TRACE("####################################\n"
				"     Video on %s is dead!!          \n"
				" No data has been received from the \n"
				"         camera for 5 seconds       \n"
				"####################################\n",
					chan2str(ch));
				continue;
			}
#endif
		}
	}

#if DEBUG_VIDEO_CALLBACK > 0
	/* Check how long the callback ran */
	gettimeofday(&tv_end, &tz);

	delta_time = (1000000*tv_end.tv_sec + tv_end.tv_usec) -
		(1000000*tv_start.tv_sec + tv_start.tv_usec);
	time_counter++;
	if (delta_time > max_delta_time)
		max_delta_time = delta_time;
	if(time_counter > 100) {
		TRACE("Worst video callback processing time for %s = %i ms\n",
				chan2str(ch), max_delta_time/1000);
		time_counter = 0;
		max_delta_time = 0;
	}
#endif

	/* (Re)submit the URB */
	if(vset->started && libusb_submit_transfer(transfer) >= 0)
		incr_active_transfers(ch);
	else
		free_video_transfer(vset->xfers, udata->xfer_idx);
}

static int alloc_transfers(video_channel_t ch, int timeout)
{
	struct libusb_transfer *transfer;
	struct video_settings *vset;
	struct video_cfg *vcfg;
	unsigned int i, ep, usb_packet_size;

	vset = get_vset(ch);
	vcfg = get_vcfg(ch);
	CHECK_ERROR(vcfg->enabled == 0, -1,
			"Unable to allocate USB transfer. %s channel is not "
			"enabled.", chan2str(ch));

	ep = vcfg->vs.alt_ep[vset->cur_alt];
	set_active_transfers(ch, 0);

	if(vset->xfers == NULL) {
		vset->xfers = calloc(num_transfers, sizeof(struct libusb_transfer*));
		CHECK_ERROR(vset->xfers == NULL, -1, "Unable to allocate the USB "
				"transfer: out of memory");
	}

	if(vcfg->vs.bulk) {
		usb_packet_size = vset->max_transfer_size;

		for(i=0; i<num_transfers; i++) {
			struct cb_user_data *user_data;
			transfer = libusb_alloc_transfer(0);
			CHECK_ERROR(transfer == NULL, -1,
					"Cannot allocate the USB transfer: "
					"out of memory");

			user_data = malloc(sizeof(struct cb_user_data));
			CHECK_ERROR(user_data == NULL, -1,
					"Unable to allocate callback "
					"buffer: out of memory");
			user_data->ch = ch;
			user_data->xfer_idx = i;

			TRACE2("Fill BULK transfer with packet_size = %i "
					"bytes.\n", usb_packet_size);

			libusb_fill_bulk_transfer(transfer,
					video_hdl,
					ep,
					calloc(1, usb_packet_size),
					usb_packet_size,
					capture_callback,
					(void*)user_data,
					timeout);
			vset->xfers[i] = transfer;
		}
	} else {
		for (i=0; i<num_transfers; i++) {
			struct cb_user_data *user_data;
			transfer = libusb_alloc_transfer(packets_per_transfer);
			CHECK_ERROR(transfer == NULL, -1,
					"Cannot allocate the USB transfer");

			user_data = malloc(sizeof(struct cb_user_data));
			CHECK_ERROR(user_data == NULL, -1,
					"Unable to allocate callback "
					"buffer: out of memory");
			user_data->ch = ch;
			user_data->xfer_idx = i;

			usb_packet_size = vset->max_transfer_size;
			TRACE2("Fill ISOC transfer with packet_size = %i bytes.\n",
					usb_packet_size);

			libusb_fill_iso_transfer(transfer,
					video_hdl,
					ep,
					calloc(packets_per_transfer, usb_packet_size),
					packets_per_transfer*usb_packet_size,
					packets_per_transfer,
					capture_callback,
					(void*)user_data,
					timeout);
			libusb_set_iso_packet_lengths(transfer, usb_packet_size);
			vset->xfers[i] = transfer;
		}
	}
	return 0;
}

static int same_guid(uint8_t guid1[16], uint8_t guid2[16])
{
	if(memcmp((const void*) guid1, (const void*) guid2, 16) == 0)
		return 1;
	else
		return 0;
}

static int find_default_frame(struct format *format, struct frame **frame)
{
	int i;

	if (format->type == VS_FORMAT_MPEG2TS) {
		*frame = NULL;
		ERROR(-1, "MPEG2TS format does not contain "
				"frame descriptors");
	}

	for (i=0; i < (int)format->num_frames; i++) {
		if(format->frames[i].id != format->default_frame_id)
			continue;
		*frame = &(format->frames[i]);
		break;
	}
	CHECK_ERROR(*frame == NULL, -1,
			"No frame corresponds to the default frame "
			"index (%i) for UVC format %s.",
			format->default_frame_id,
			uvcformat2str(format->id));

	return 0;
}

static int probe_commit_settings(video_channel_t ch)
{
	int ret, i, found;
	struct video_settings *vset;
	struct video_cfg *vcfg;

	vset = get_vset(ch);
	vcfg = get_vcfg(ch);

	CHECK_ERROR(vcfg->enabled == 0, -1,
			"Unable to probe the camera. %s channel is not "
			"enabled", chan2str(ch));

	/* Probe the camera */
	struct uvc_stream_params params;
	memset(&params, 0, sizeof(params));
	params.bFormatIndex = vset->cur_format->id;
	params.bFrameIndex = vset->cur_frame == 0 ? 0 : vset->cur_frame->id;
	params.dwFrameInterval = vset->cur_fri;;

	ret = probe_strm_params(ch, &params, SET_CUR);
	CHECK_ERROR(ret < 0, -1, "Failed to probe the camera while setting.");

	ret = probe_strm_params(ch, &params, GET_CUR);
	CHECK_ERROR(ret < 0, -1, "Failed to probe the camera while setting.");

	vset->max_transfer_size = params.dwMaxPayloadTransferSize;
	vset->max_frame_size = params.dwMaxVideoFrameSize;

	/* Find the alternate settings corresponding to the
	 * params requested */
	found = 0;
	if(vcfg->vs.bulk) {
		vset->cur_alt = 0;
		found = 1;
	}
	for(i=1; i <= (int)vcfg->vs.num_video_alt; i++) {
		if (found)
			break;
		if(vset->max_transfer_size !=
				vcfg->vs.alt_maxpacketsize[i])
			continue;
		vset->cur_alt = i;
		found = 1;
	}
	CHECK_ERROR(found == 0, -1, "Failed to commit changes to the camera: "
			"could not find an alternate settings for "
			"dwMaxPayloadTransferSize = %i",
			params.dwMaxPayloadTransferSize);

	/* Commit changes */
	ret = commit_strm_params(ch, &params, SET_CUR);
	CHECK_ERROR(ret < 0, -1, "Failed to commit changes to the camera.");

	return 0;
}

static int controls_init(video_channel_t ch)
{
	VIDEO_CTRL *ctrl = uvc_controls;
	int len, ret, found;
	int64_t min = 0, max = 0;
	uint64_t bitmap;
	struct control_unit_ext *ext_unit;
	struct video_cfg *vcfg;

	vcfg = get_vcfg(ch);

	while(ctrl->id) {
		switch(ctrl->cfg.type) {
		case PROC:
			ctrl->settings[ch].unit = vcfg->vc.processing_unit.id;
			bitmap = vcfg->vc.processing_unit.bmControls;
			break;
		case CAM:
			ctrl->settings[ch].unit = vcfg->vc.camera_unit.id;
			bitmap = vcfg->vc.camera_unit.bmControls;
			break;
		case AVCEXT:
			found = 0;
			ext_unit = vcfg->vc.extension_unit;
			while(ext_unit) {
				if (same_guid(ext_unit->guid, guid_avcext)) {
					ctrl->settings[ch].unit = ext_unit->id;
					bitmap = ext_unit->bmControls;
					found = 1;
					break;
				}
				ext_unit = ext_unit->next;
			}
			if(!found)
				goto next_ctrl;

			break;
		case PUEXT:
			found = 0;
			ext_unit = vcfg->vc.extension_unit;
			while(ext_unit) {
				if (same_guid(ext_unit->guid, guid_puext)) {
					ctrl->settings[ch].unit = ext_unit->id;
					bitmap = ext_unit->bmControls;
					found = 1;
					break;
				}
				ext_unit = ext_unit->next;
			}
			if(!found)
				goto next_ctrl;

			break;
		default:
			ERROR(-1, "Unsupported control type (%i) for video "
				"control %s", ctrl->cfg.type, ctrl2str(ctrl->id));
		}

		/* Sanity check */
		CHECK_ERROR(ctrl->cfg.bit_bmcontrols >= 64, -1,
			"%s: bmControls size is greater than 64 bits (%i).",
			ctrl2str(ctrl->id), ctrl->cfg.bit_bmcontrols);

		/* Dectect if the control is enabled or disabled */
		if((bitmap >> ctrl->cfg.bit_bmcontrols) & 0x1) {
			ctrl->settings[ch].enabled = 1;
			TRACE2("%s enabled on %s channel.\n",
					ctrl2str(ctrl->id), chan2str(ch));
		} else {
			ctrl->settings[ch].enabled = 0;
			TRACE("%s disabled on %s channel.\n",
					ctrl2str(ctrl->id), chan2str(ch));
			goto next_ctrl;
		}

		/* Get len, min, max */
		if (ctrl->cfg.type == AVCEXT || ctrl->cfg.type == PUEXT ) {
			ret = get_ctrl(ch, ctrl->id, GET_LEN,
					(void*) &len);
			CHECK_ERROR(ret < 0, -1,
				"Unable to get length (GET_LEN) for %s",
				ctrl2str(ctrl->id));
			ctrl->cfg.len = len;
		}
		ret = get_ctrl(ch, ctrl->id, GET_MIN, (void*) &min);
		CHECK_ERROR(ret < 0, -1,
				"Unable to get min (GET_MIN) for %s",
				ctrl2str(ctrl->id));
		ctrl->settings[ch].min = min;
		ret = get_ctrl(ch, ctrl->id, GET_MAX, (void*) &max);
		CHECK_ERROR(ret < 0, -1,
				"Unable to get max (GET_MAX) for %s",
				ctrl2str(ctrl->id));
		ctrl->settings[ch].max = max;

next_ctrl:
		ctrl++;
	}

	/* Set the cache with the current values of pan and tilt */
	struct video_settings *vset = get_vset(ch);
	pthread_mutex_init(&(vset->pantilt_mutex), NULL);
	pthread_mutex_lock(&(vset->pantilt_mutex));
	//get_pantilt(ch, &(vset->cache_pan), &(vset->cache_tilt));
	pthread_mutex_unlock(&(vset->pantilt_mutex));

	return 0;
}

static void video_removed(int fd, void *user_data)
{
	TRACE("Video removed notification.\n");
	video_disconnected = 1;
}

int mxuvc_video_init(const char *backend, const char *options, int fd)
{
    ////("tttttttttttt1");
	//RECORD("\"%s\", \"%s\"", backend, options);
	struct libusb_device *dev = NULL;
	int ret=0, i, config;
	video_channel_t chan;
	uint16_t vendor_id = 0xdead, product_id = 0xbeef;
	char *str = NULL, *opt, *value;

	////("Initializing the video\n");

	/* Check that the correct video backend was requested*/
	if(strncmp(backend, "libusb-uvc", 10)) {
		ERROR(-1, "The video backend requested (%s) does not match "
			"the implemented one (libusb-uvc)", backend);
	}

	/* Set init parameters to their default values */
	packets_per_transfer  = PACKETS_PER_TRANSFER_DEFAULT;
	num_transfers         = NUM_TRANSFERS_DEFAULT;
	check_h264_continuity = CHECK_H264_CONTINUITY_DEFAULT;

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
		} else if(strncmp(opt, "num_transfers", 13) == 0) {
			num_transfers = (unsigned int) strtoul(value, NULL, 10);
		} else if(strncmp(opt, "check_h264_continuity", 21) == 0) {
			check_h264_continuity = (unsigned int) strtoul(value, NULL, 10);
		} else {
			WARNING("Unrecognized option: '%s'", opt);
		}
		ret = next_opt(NULL, &opt, &value);
	}

	/* Display the values we are going to use */
	////("Using vid = 0x%x\n",                  vendor_id);
	////("Using pid = 0x%x\n",                  product_id);
	////("Using packets_per_transfer = %i\n",   packets_per_transfer);
	////("Using num_transfers = %u\n",          num_transfers);
	////("Using check_h264_continuity = %i\n",  check_h264_continuity);

	/* Free the memory allocated to parse 'options' */
	if (str)
		free(str);
    ////("//////OL");
	/* Initialize the backend */
	video_disconnected = 0;
	ret = init_libusb(&video_ctx);
	////("//////1L");
	if(ret < 0)
		return -1;
	libusb_device_handle *usb_devh = 0;
	////("//////2L");
	int result = libusb_wrap_fd(video_ctx,fd,&usb_devh);
	////("//////4L");
	video_hdl = usb_devh;
	////("//////3L");
	dev = libusb_get_device(video_hdl);
	if(dev == NULL) {
		printf("Unexpected error: libusb_get_device returned a NULL "
				"pointer.");
		mxuvc_video_deinit();
		return -1;
	}
    ////("//////a");
	/* Get active USB configuration */
	libusb_get_configuration(video_hdl, &config);

	/* Parse USB decriptors from active USB configuration
	 * to get all the UVC/UAC info needed */
	ret = parse_usb_config(dev, config);
	if(ret < 0){
		mxuvc_video_deinit();
		return -1;
	}
    ////("//////b");
	/* Initialize video */

	/* Claim Video Control/Streaming Interfaces and get default settings */
	for(chan=0; chan<NUM_VID_CHANNEL; chan++) {
		int int_num[2];
		struct video_cfg *vcfg;
		struct video_settings *vset;

		vset = get_vset(chan);
		vcfg = get_vcfg(chan);

		/* Skip if channel is not enabled */
		if(vcfg->enabled == 0)
			continue;

		int_num[0] = vcfg->vc.interface;
		int_num[1] = vcfg->vs.interface;

		for(i=0; i<2; i++) {
            /* Check if a kernel driver is active on the interface */
            ret = libusb_kernel_driver_active(video_hdl, int_num[i]);
            CHECK_ERROR(ret == LIBUSB_ERROR_NO_DEVICE, -1,
                "Unable to detect if USB insterface %i is currently "
                "been used by a kernel driver. The USB device havs "
                "been disconnected.", int_num[i]);
            CHECK_ERROR(ret < 0, -1,
                "Unable to detect if USB insterface %i is currently "
                "been used by a kernel driver. Libusb return code is "
                "%i.", int_num[i], ret);

            if(ret == 1) {
                TRACE("Detaching the kernel driver...\n");
                /* A kernel driver is active, detach it */
                ret = libusb_detach_kernel_driver(video_hdl, int_num[i]);
                CHECK_ERROR(ret == LIBUSB_ERROR_INVALID_PARAM, -1,
                    "The requested USB interface to detach (%i) "
                    "does not exist.", int_num[i]);
                CHECK_ERROR(ret == LIBUSB_ERROR_NO_DEVICE, -1,
                    "Unable to detach USB interface %i. The "
                    "device has been disconnected.", int_num[i]);
                CHECK_ERROR(ret < 0 && ret != LIBUSB_ERROR_NOT_FOUND, -1,
                    "Unable to detach the kernel driver currently "
                    "using USB interface %i. Libusb return code "
                    "%i.", int_num[i], ret);
            }

            /* Claim the interface */
            ret = libusb_claim_interface(video_hdl, int_num[i]);
            CHECK_ERROR(ret == LIBUSB_ERROR_NOT_FOUND, -1,
                "The requested USB interface %i does not exist.",
                int_num[i]);
            CHECK_ERROR(ret == LIBUSB_ERROR_BUSY, -1,
                "The requested USB interface %i has already been "
                "claimed by another driver or program.", int_num[i]);
            CHECK_ERROR(ret == LIBUSB_ERROR_NO_DEVICE, -1,
                "Unable to claim USB interface %i. The device has "
                "been disconnected.", int_num[i]);
            CHECK_ERROR(ret < 0, -1,
                "Unable to claim USB interface %i. Libusb return code "
                "is %i.", int_num[i], ret);
		}

		/* Get default settings for Video Streaming */
		TRACE("Getting default video settings for %s channel\n",
				chan2str(chan));
		CHECK_ERROR(vcfg->vs.num_formats == 0, -1,
				"No streaming format found for %si channel.",
				chan2str(chan));
		vset->cur_format = &(vcfg->vs.formats[0]);

		vset->cur_frame = NULL;
		vset->cur_fri = 0;

		if (vset->cur_format->type != VS_FORMAT_MPEG2TS) {
			for (i=0; i < (int)vset->cur_format->num_frames; i++) {
				if(vset->cur_format->frames[i].id !=
					vset->cur_format->default_frame_id)
					continue;
				vset->cur_frame = &(vset->cur_format->frames[i]);
				break;
			}
			CHECK_ERROR(vset->cur_frame == NULL, -1,
				"No frame corresponds to the default frame "
				"index (%i) for UVC format %s.",
				vset->cur_format->default_frame_id,
				uvcformat2str(vset->cur_format->id));

			vset->cur_fri = vset->cur_frame->default_fri;
		}

		/* Map default UVC format to Video Format */
		switch(vset->cur_format->type) {
		case VS_FORMAT_MPEG2TS:
			vset->cur_video_format = VID_FORMAT_H264_TS;
			break;
		case VS_FORMAT_MJPEG:
			vset->cur_video_format = VID_FORMAT_MJPEG_RAW;
			break;
		case VS_FORMAT_FRAME_BASED:
			if(same_guid(vset->cur_format->guid_format,
						guid_raw_h264))
				vset->cur_video_format = VID_FORMAT_H264_RAW;
			else
				ERROR(-1, "Unable to initialize the video "
					"for %s channel. Unknown default "
					"format guid", chan2str(chan));
			break;
		case VS_FORMAT_UNCOMPRESSED:
			if(same_guid(vset->cur_format->guid_format,
						guid_yuy2))
				vset->cur_video_format = VID_FORMAT_YUY2_RAW;
			else if (same_guid(vset->cur_format->guid_format,
						guid_nv12))
				vset->cur_video_format = VID_FORMAT_NV12_RAW;
			else
				ERROR(-1, "Unable to initialize the video "
					"for %s channel. Unknown default "
					"format guid", chan2str(chan));
			break;
		default:
			ERROR(-1, "Unsupported UVC format: %i",
					vset->cur_format->type);
		}

		/* Initialize the camera controls */
		ret = controls_init(chan);
		CHECK_ERROR(ret < 0, -1, "Failed to initialize the controls");
	}

	/* Register removal USB event*/
	register_libusb_removal_cb((libusb_pollfd_removed_cb) video_removed,
				video_hdl);

	/* Start event thread/loop */
	ret = start_libusb_events();
	if(ret < 0)
		return -1;
	video_initialized = 1;
	return 0;
}

int mxuvc_video_deinit()
{
	int ret, i, j, int_num[2];
	video_channel_t chan;
	struct video_settings *vset;
	struct video_cfg *vcfg;
	struct control_unit_ext *ext_unit;
	struct format *format;
	struct frame *frame;
    ////("[[[1");
	if(!video_initialized)
		return 1;
    ////("[[[2");
	deregister_libusb_removal_cb((libusb_pollfd_removed_cb) video_removed);
    ////("[[[3");
	/* Deinit video */

	/* Release Video Control and Streaming interfaces */
	for(chan=0; chan<NUM_VID_CHANNEL; chan++) {
		vset = get_vset(chan);
		vcfg = get_vcfg(chan);

		/* Skip channel if channel is not enabled */
		if(vcfg==0 || vcfg->enabled == 0)
			continue;

		if(!video_disconnected) {
			/* Stop the video first if not stopped yet */
			if(vset!=0 && vset->started == 1)
				mxuvc_video_stop(chan);

			/* Video Control Interface */
			int_num[0] = vcfg->vc.interface;
			/* Currently used Video Streaming Interface */
			int_num[1] = vcfg->vs.interface;
			for(i=0; i<2; i++) {
				/* Release interface */
				TRACE2("Releasing video interface %i\n", int_num[i]);
				ret = libusb_release_interface(video_hdl, int_num[i]);
				/* We only fail in case of an unexpected failure. */
				/* We return success (0) if the interface hasn't been
				 * claimed or of the device has been disconnected */
				CHECK_ERROR(ret < 0 && ret != LIBUSB_ERROR_NOT_FOUND
						&& ret != LIBUSB_ERROR_NO_DEVICE, -1,
						"Unable to release USB interface %i. Libusb "
						"return code is: %i", int_num[i], ret);
			}
			/* Re-attach the previously attached kernel driver */
			for(i=0; i<2; i++)
				libusb_attach_kernel_driver(video_hdl, int_num[i]);
		}

		/* Free mxuvc resources */
		//TRACE("Freeing resources for %s channel\n", chan2str(chan));
		while(vcfg->vc.extension_unit) {
			ext_unit = vcfg->vc.extension_unit;
			vcfg->vc.extension_unit = vcfg->vc.extension_unit->next;
			free(ext_unit);
		}
		if(!vcfg->vs.formats)
			continue;
		for(i=0; i<(int)vcfg->vs.num_formats; i++) {
			format = vcfg->vs.formats + i;
			for(j = 0; j < (int)format->num_frames; j++) {
				frame = format->frames + j;
				if(frame->fri.type == FRI_DISCRETE
						&& frame->fri.discretes) {
						if(frame->fri.num>0){
					       // free(frame->fri.discretes);
					    }
					    frame->fri.num = 0;
					frame->fri.discretes = 0;
				}
			}
			if(format->frames)
				free(format->frames);
		}
		free(vcfg->vs.formats);
	}
	//("[[[4");
	/* Exit Camera Event loop/thread */
	stop_libusb_events();
	/* Free USB resources */
	//TRACE("Freeing USB video resources\n");
	//if(mxuvc_video_alive())
		libusb_close(video_hdl);
	exit_libusb(&video_ctx);
	//("[[[5");
	video_hdl = NULL;
	video_ctx = NULL;
	for(chan=0; chan<NUM_VID_CHANNEL; chan++) {
		vset = get_vset(chan);
		if (vset!=0 && vset->xfers) {
			free(vset->xfers);
			vset->xfers = NULL;
		}
	}
	//("[[[6");
	/* Destroy pan/tilt mutex */
	if(vset!=0){
	    pthread_mutex_destroy(&(vset->pantilt_mutex));
    }
	video_initialized = 0;
	TRACE("The video has been successfully uninitialized\n");
    //("[[[7");
	//mxuvc_debug_stoprec();
	return 0;
}

int mxuvc_video_alive()
{
	RECORD("");
	if (!video_disconnected)
		return 1;
	else
		return 0;
}

int mxuvc_video_start(video_channel_t ch)
{
	//RECORD("%s", chan2str(ch));
	int ret;
	unsigned int i;
	struct video_settings *vset;
	struct video_cfg *vcfg;
    //("]]]]1");

	vset = get_vset(ch);
	vcfg = get_vcfg(ch);
    //("]]]]2");
    if(!vcfg->vs.bulk)
        CHECK_ERROR(packets_per_transfer>MAX_PACKETS_PER_ISOC_TRANSFER, -1, "exceeded the max packets per iso transfer");
    //("]]]]3");
	/* Probe/Commit to find the alternate setting corresponding
	 * to the default format/frame */
	ret = probe_commit_settings(ch);
    //("]]]]4");
	if(vcfg->vs.bulk) {
		VIDEO_CTRL *ctrl;
		ctrl = get_ctrl_by_id(CTRL_START_SKYPE_BULK_CHANNEL);
		ret = set_ctrl(ch, CTRL_START_SKYPE_BULK_CHANNEL, SET_CUR, (void*) &ch);
	}
	//("]]]]5");
	/* Initialize the USB transfers */
	ret = alloc_transfers(ch, USB_TIMEOUT);
	if(ret < 0)
		return -1;
    //("]]]]6");
	/* Switch alternate settings for ISOC */
	if(!vcfg->vs.bulk) {
		ret = libusb_set_interface_alt_setting(video_hdl,
			vcfg->vs.interface, vset->cur_alt);
	}
	//("]]]]7");
	vset->frame_count = 0;
	vset->first_transfer = 1;
	vset->started = 1;
    //("]]]]8");
	if(gFrameBuf1[ch] != NULL)
		free(gFrameBuf1[ch]);
	if(gFrameBuf2[ch] != NULL)
		free(gFrameBuf2[ch]);
    //("]]]]9");
	gFrameBuf1[ch] = (unsigned char*) malloc(vset->max_frame_size);
	if(gFrameBuf1[ch]  == NULL) {
		return -1;
	}
	gFrameBuf2[ch] = (unsigned char*) malloc(vset->max_frame_size);
	if(gFrameBuf2[ch]  == NULL) {
		free(gFrameBuf1[ch]);
		return -1;
	}
    //("]]]]10");
	/* Submit the USB tranfers */
	for (i=0; i<num_transfers; i++) {
		ret = libusb_submit_transfer(vset->xfers[i]);
		if(ret < 0) {
			free(gFrameBuf1[ch]);
			free(gFrameBuf2[ch]);
			free_video_transfer(vset->xfers, i);
		}
		incr_active_transfers(ch);
	}
	//("]]]]11");
	return 0;
}

int mxuvc_video_stop(video_channel_t ch)
{
	struct video_settings *vset;
	struct video_cfg *vcfg;
	unsigned int c=0, i;

	vset = get_vset(ch);
	vcfg = get_vcfg(ch);
	vset->started = 0;
    //("mmmmm11");
	if (!video_disconnected) {
		while(get_active_transfers(ch)) {
			if(c >= 1000000) {
				for(i=0; i<num_transfers; i++)
					if(vset->xfers[i] != NULL)
						libusb_cancel_transfer(vset->xfers[i]);
			}
			usleep(3000);
			c+=3000;
		}
		auto ret = libusb_set_interface_alt_setting(video_hdl,
				vcfg->vs.interface, 0);
	} else {
		while(get_active_transfers(ch))
			usleep(3000);
	}

	if(gFrameBuf1[ch] != NULL) {
		free(gFrameBuf1[ch]);
		gFrameBuf1[ch] = NULL;
	}

	if(gFrameBuf2[ch] != NULL) {
		free(gFrameBuf2[ch]);
		gFrameBuf2[ch] = NULL;
	}
    //("mmmmm12");
	return 0;
}

int mxuvc_video_register_cb(video_channel_t ch, mxuvc_video_cb_t func, void* user_data)
{
    //("mmmmm13");
	struct video_settings *vset;
	struct video_cfg *vcfg;
    //("mmmmm14");
	vset = get_vset(ch);
	vcfg = get_vcfg(ch);
	if(vset != 0){
        vset->cb = func;
        vset->cb_user_data = user_data;
	}
    //("mmmmm15");
	return 0;
}

int mxuvc_video_set_format(video_channel_t ch, video_format_t fmt)
{
    //("mmmmm16");
	int i, ret, found, restart;
	struct video_settings *vset;
	struct video_cfg *vcfg;
	struct format *formats;

	vset = get_vset(ch);
	vcfg = get_vcfg(ch);
    if(vset ==0 || vcfg == 0){
        return 0;
    }
	if (vset->cur_video_format == fmt)
		return 0;

	found = 0;
	switch(fmt) {
	case VID_FORMAT_H264_RAW:
		if(check_h264_continuity) {
			formats = vcfg->vs.formats;
			for (i=0; i < (int)vcfg->vs.num_formats; i++) {
				if(formats[i].type != VS_FORMAT_MPEG2TS)
					continue;
				vset->cur_format = &(formats[i]);
				found = 1;
			}
			vset->quirk_demuxts = 1;
		} else {
			formats = vcfg->vs.formats;
			for (i=0; i < (int)vcfg->vs.num_formats; i++) {
				if(formats[i].type != VS_FORMAT_FRAME_BASED)
				if(!same_guid(formats[i].guid_format, guid_raw_h264))
					continue;
				vset->cur_format = &(formats[i]);
				found = 1;
			}
		}
		break;
	case VID_FORMAT_H264_AAC_TS:
	case VID_FORMAT_H264_TS:
		formats = vcfg->vs.formats;
		for (i=0; i < (int)vcfg->vs.num_formats; i++) {
			if(formats[i].type != VS_FORMAT_MPEG2TS)
				continue;
			vset->cur_format = &(formats[i]);
			found = 1;
		}
		break;
	case VID_FORMAT_MJPEG_RAW:
		formats = vcfg->vs.formats;
		for (i=0; i < (int)vcfg->vs.num_formats; i++) {
			if(formats[i].type != VS_FORMAT_MJPEG)
				continue;
			vset->cur_format = &(formats[i]);
			found = 1;
		}
		break;
	case VID_FORMAT_YUY2_RAW:
		formats = vcfg->vs.formats;
		for (i=0; i < (int)vcfg->vs.num_formats; i++) {
			if(formats[i].type != VS_FORMAT_UNCOMPRESSED)
				continue;
			if(!same_guid(formats[i].guid_format, guid_yuy2))
				continue;
			vset->cur_format = &(formats[i]);
			found = 1;
		}
		break;
	case VID_FORMAT_NV12_RAW:
		formats = vcfg->vs.formats;
		for (i=0; i < (int)vcfg->vs.num_formats; i++) {
			if(formats[i].type != VS_FORMAT_UNCOMPRESSED)
				continue;
			if(!same_guid(formats[i].guid_format, guid_nv12))
				continue;
			vset->cur_format = &(formats[i]);
			found = 1;
		}
		break;
	default:
		;
	}

	/* Quirk: if no RAW H264 support, set format to TS and demux */
	if (fmt == VID_FORMAT_H264_RAW && found == 0) {
		formats = vcfg->vs.formats;
		for (i=0; i < (int)vcfg->vs.num_formats; i++) {
			if(formats[i].type != VS_FORMAT_MPEG2TS)
				continue;
			vset->cur_format = &(formats[i]);
			found = 1;
			vset->quirk_demuxts = 1;
		}
	}

	vset->cur_video_format = fmt;

	/* Find default frame and frame rate corresponding to the format */
	vset->cur_fri = 0;
	if(vset->cur_format->type == VS_FORMAT_MPEG2TS)
		vset->cur_frame = NULL;
	else {
		ret = find_default_frame(vset->cur_format, &(vset->cur_frame));
		vset->cur_fri = vset->cur_frame->default_fri;
	}

	/* Stop video streaming if necessary before changing format */
	restart = 0;
	if(vset->started) {
		restart = 1;
		ret = mxuvc_video_stop(ch);
	}

	/* Enable AAC muxing if necessary */
	if((vset->cur_format->type == VS_FORMAT_MPEG2TS) &&
		(((fmt == VID_FORMAT_H264_AAC_TS) && (vset->mux_aac_ts == 0)) ||
		((fmt != VID_FORMAT_H264_AAC_TS) && (vset->mux_aac_ts != 0)))) {

		int value;
		if (fmt == VID_FORMAT_H264_AAC_TS)
			value = 1;
		else
			value = 0;

		ret = set_ctrl(ch, CTRL_AVMUX, SET_CUR, (void*) &value);
		vset->mux_aac_ts = value;
	}

	/* Restart video streaming if necessary */
	if(restart) {
		ret = mxuvc_video_start(ch);
	}
    ////("mmmmm17");
	return 0;
}

int mxuvc_video_get_format(video_channel_t ch, video_format_t *fmt)
{
	RECORD("%s, %p", chan2str(ch), fmt);
	TRACE2("Getting the video format on %s channel.\n", chan2str(ch));

	struct video_settings *vset;
	struct video_cfg *vcfg;

	vset = get_vset(ch);
	vcfg = get_vcfg(ch);
	CHECK_ERROR(vcfg->enabled == 0, -1,
			"Unable to get the video format for %s "
			"channel. The channel is not enabled.", chan2str(ch));

	*fmt = vset->cur_video_format;

	return 0;
}


int mxuvc_video_set_resolution(video_channel_t ch, uint16_t width, uint16_t height)
{
	int ret, i, found, res;
	struct video_settings *vset;
	struct video_cfg *vcfg;
	struct frame *frame;
	vset = get_vset(ch);
	vcfg = get_vcfg(ch);
    if(vset == 0 || vcfg == 0){
        return 0;
    }
	switch(vset->cur_video_format) {
	/* For H264 formats, we use the extension to change resolution */
	case VID_FORMAT_H264_RAW:
	case VID_FORMAT_H264_TS:
	case VID_FORMAT_H264_AAC_TS:
		res = (width << 16) + height;
		ret = set_ctrl(ch, CTRL_RESOLUTION, SET_CUR, (void*) &res);
		break;
	/* For the rest ... */
	default:
		found = 0;
		for(i=0; i < (int)vset->cur_format->num_frames; i++) {
			frame = &(vset->cur_format->frames[i]);
			if (frame->width != width || frame->height != height)
				continue;
			vset->cur_frame = frame;
			vset->cur_fri = frame->default_fri;
			found = 1;
			break;
		}

		ret = probe_commit_settings(ch);
		CHECK_ERROR(ret < 0, -1, "Unable to set resolution.");
	}
	return 0;
}

int mxuvc_video_set_framerate(video_channel_t ch, uint32_t framerate)
{
	RECORD("%s, %i", chan2str(ch), framerate);
	int j, ret;
	uint32_t interval, new_fri = 0;
	struct video_settings *vset;
	struct video_cfg *vcfg;
	struct frame *frame;
	struct frame_interval *fri;

	TRACE("Setting framerate to %i on %s channel.\n", framerate,
			chan2str(ch));

	vset = get_vset(ch);
	vcfg = get_vcfg(ch);
	CHECK_ERROR(vcfg->enabled == 0, -1,
			"Unable to set the framerate on %s channel: the "
			"channel is not enabled.", chan2str(ch));

	CHECK_ERROR(framerate == 0, -1, "Cannot set framerate to 0.");

	interval = 10000000/framerate;

	if (vset->cur_video_format == VID_FORMAT_H264_TS
			|| vset->cur_video_format == VID_FORMAT_H264_AAC_TS
			|| vset->quirk_demuxts) {
		/* No check can be done on the framerate for TS format */
		vset->cur_fri = interval;
		goto probe_commit;
	}
	frame = &(vset->cur_format->frames[vset->cur_frame->id-1]);
	fri = &frame->fri;

	switch(fri->type) {
	case FRI_CONTINUOUS:
		if(interval >= fri->min && interval <= fri->max)
			new_fri = interval;
		else {
			WARNING("The framerate requested (%i) is "
				"greater than the maximum framerate "
				"supported by the current format. "
				"Using the maximum framerate (%i)",
				framerate, 10000000/fri->max);
			new_fri = fri->max;
		}
		break;
	case FRI_DISCRETE:
		for (j=0; j< (int)(fri->num); j++) {
			new_fri = fri->discretes[j];
			if(interval == fri->discretes[j])
				break;
			if(interval < fri->discretes[j] ||
					j == (int)(fri->num - 1)) {
				WARNING("The framerate requested (%i) "
					"is not supported for the "
					"current format. Setting the "
					"framerate to the closest "
					"supported one: %i", framerate,
					new_fri);
				break;
			}
		}
		break;
	default:
		ERROR(-1, "Unexpected frame interval type: %i", fri->type);
	}
	vset->cur_fri = new_fri;

probe_commit:
	ret = probe_commit_settings(ch);
	CHECK_ERROR(ret < 0, -1, "Unable to set framerate.");

	return 0;
}

int mxuvc_video_get_framerate(video_channel_t ch, uint32_t *framerate)
{
	RECORD("%s, %p", chan2str(ch), framerate);
	int ret;
	struct uvc_stream_params params;
	struct video_cfg *vcfg;

	TRACE("Getting framerate on %s channel.\n", chan2str(ch));

	vcfg = get_vcfg(ch);
	CHECK_ERROR(vcfg->enabled == 0, -1,
			"Unable to get the framerate on %s channel: "
			"the channel is not enabled.", chan2str(ch));

	/* Probe the camera */
	memset(&params, 0, sizeof(params));
	ret = probe_strm_params(ch, &params, GET_CUR);
	CHECK_ERROR(ret < 0, -1, "Unable to get the framerate on %s channel: "
			"failed to probe the camera.", chan2str(ch));

	*framerate = 10000000/params.dwFrameInterval;

	return 0;
}

int mxuvc_video_force_iframe(video_channel_t ch)
{
	RECORD("%s", chan2str(ch));
	int ret;
	struct video_settings *vset;
	struct video_cfg *vcfg;
	int value = 1;

	TRACE("Forcing I frame on %s channel.\n", chan2str(ch));

	vset = get_vset(ch);
	vcfg = get_vcfg(ch);
	CHECK_ERROR(vcfg->enabled == 0, -1,
			"Unable to force I frame on %s channel: the channel "
			"is not enabled.", chan2str(ch));

	switch(vset->cur_video_format) {
		case VID_FORMAT_H264_RAW:
		case VID_FORMAT_H264_TS:
		case VID_FORMAT_H264_AAC_TS:
			ret = set_ctrl(ch, CTRL_IFRAME, SET_CUR, (void*) &value);
			CHECK_ERROR(ret < 0, -1, "Unable to force I frame on "
					"%s channel.", chan2str(ch));
			break;
		default:
			ERROR(-1, "Cannot force I frame for the current "
					"format (%i) on %s channel",
					vset->cur_video_format, chan2str(ch));
	}

	return 0;
}

#if 0 /* NOT SUPPORTED */
static int set_pantilt(video_channel_t ch, int32_t *pan, int32_t *tilt)
{
	int ret;
	int32_t value[2];
	struct video_cfg *vcfg;
	struct video_settings *vset;

	vset = get_vset(ch);
	vcfg = get_vcfg(ch);
	CHECK_ERROR(vcfg->enabled == 0, -1,
			"Unable to set the pan/tilt. %s channel is not "
			"enabled.", chan2str(ch));

	pthread_mutex_lock(&(vset->pantilt_mutex));
	if(pan == NULL)
		value[0] = vset->cache_pan;
	else
		value[0] = *pan;

	if(tilt == NULL)
		value[1] = vset->cache_tilt;
	else
		value[1] = *tilt;

	ret = set_ctrl(ch, CTRL_PANTILT_ABS, SET_CUR, (void*) &value);
	if(ret >=0) {
		vset->cache_pan = value[0];
		vset->cache_tilt = value[1];
	}
	pthread_mutex_unlock(&(vset->pantilt_mutex));

	CHECK_ERROR(ret < 0, -1, "Unable to change the "
	"pan/tilt on %s channel.", chan2str(ch));

	return 0;
}

static int get_pantilt(video_channel_t ch, int32_t *pan, int32_t *tilt)
{
	int ret;
	int32_t value[2];
	struct video_cfg *vcfg;

	vcfg = get_vcfg(ch);
	CHECK_ERROR(vcfg->enabled == 0, -1,
			"Unable to get the pan/til. %s channel is not "
			"enabled.", chan2str(ch));


	ret = get_ctrl(ch, CTRL_PANTILT_ABS, GET_CUR, (void*) &value);
	CHECK_ERROR(ret < 0, -1, "Unable to get the "
	"pan/tilt on %s channel.", chan2str(ch));

	*pan = value[0];
	*tilt = value[1];

	return 0;
}

int mxuvc_video_set_pantilt(video_channel_t ch, int32_t pan, int32_t tilt)
{
	RECORD("%s, %i, %i", chan2str(ch), pan, tilt);

	TRACE("Setting pan/tilt to %i/%i on %s channel.\n", pan, tilt,
			chan2str(ch));

	return set_pantilt(ch, &pan, &tilt);
}
int mxuvc_video_set_pan(video_channel_t ch, int32_t pan)
{
	RECORD("%s, %i", chan2str(ch), pan);
	TRACE("Setting pan to %i on %s channel.\n", pan,
			chan2str(ch));

	return set_pantilt(ch, &pan, NULL);
}
int mxuvc_video_set_tilt(video_channel_t ch, int32_t tilt)
{
	RECORD("%s, %i", chan2str(ch), tilt);
	TRACE("Setting tilt to %i on %s channel.\n", tilt,
			chan2str(ch));

	return set_pantilt(ch, NULL, &tilt);
}

int mxuvc_video_get_pantilt(video_channel_t ch, int32_t *pan, int32_t *tilt)
{
	RECORD("%s, %p, %p", chan2str(ch), pan, tilt);

	TRACE("Getting pan/tilt on %s channel.\n", chan2str(ch));

	return get_pantilt(ch, pan, tilt);
}
int mxuvc_video_get_pan(video_channel_t ch, int32_t *pan)
{
	int32_t tmp;
	RECORD("%s, %p", chan2str(ch), pan);

	TRACE("Getting pan on %s channel.\n", chan2str(ch));

	return get_pantilt(ch, pan, &tmp);
}
int mxuvc_video_get_tilt(video_channel_t ch, int32_t *tilt)
{
	int32_t tmp;
	RECORD("%s, %p", chan2str(ch), tilt);

	TRACE("Getting tilt on %s channel.\n", chan2str(ch));

	return get_pantilt(ch, &tmp, tilt);
}

#endif

int mxuvc_video_set_wdr(video_channel_t ch, wdr_mode_t mode, uint8_t value)
{
	RECORD("%s, %i, %i", chan2str(ch), mode, value);
	int ret;
	struct video_cfg *vcfg;

	TRACE("Setting wdr to mode - %i value - %i on channel %s\n",mode, value, chan2str(ch));

	vcfg = get_vcfg(ch);
	CHECK_ERROR(vcfg->enabled == 0, -1,
			"Unable to set the wdr on %s channel: the "
			"channel is not enabled.", chan2str(ch));

	if ( mode == WDR_AUTO )
		mode = 1;
	else
		mode = 0;

	ret = set_ctrl(ch, CTRL_ADAPTIVE_WDR_ENABLE, SET_CUR, (void*) &mode);

	CHECK_ERROR(ret < 0, -1, "Unable to change the WDR mode on %s channel.", chan2str(ch));

	ret = set_ctrl(ch, CTRL_WDR_STRENGTH, SET_CUR, (void*) &value);

	CHECK_ERROR(ret < 0, -1, "Unable to change the WDR strength on %s channel.", chan2str(ch));

	return 0;
}


int mxuvc_video_get_wdr(video_channel_t ch, wdr_mode_t *mode, uint8_t *value)
{
	RECORD("%s, %p, %p", chan2str(ch), mode, value);
	int ret;
	struct video_cfg *vcfg;
	uint8_t mode_value = 0;

	TRACE("Getting WDR on %s channel.\n", chan2str(ch));

	vcfg = get_vcfg(ch);
	CHECK_ERROR(vcfg->enabled == 0, -1,
			"Unable to get the WDR on %s channel: the "
			"channel is not enabled.", chan2str(ch));

	ret = get_ctrl(ch, CTRL_ADAPTIVE_WDR_ENABLE, GET_CUR, (void*) &mode_value);
	CHECK_ERROR(ret < 0, -1, "Unable to get the WDR mode on "
			"%s channel.", chan2str(ch));
	if ( mode_value )
		*mode = WDR_AUTO;
	else
		*mode = WDR_MANUAL;

	ret = get_ctrl(ch, CTRL_WDR_STRENGTH, GET_CUR, (void*) value);
	CHECK_ERROR(ret < 0, -1, "Unable to get the WDR strength on "
			"%s channel.", chan2str(ch));

	return 0;
}

int mxuvc_get_camera_mode(camer_mode_t *mode)
{
	*mode = SKYPE;
	return 0;
}

int mxuvc_video_get_channel_count(uint32_t *count)
{
	*count = 2;
	return 0;
}

int mxuvc_video_get_channel_info(video_channel_t ch, video_channel_info_t *info)
{
	mxuvc_video_get_format(ch, &info->format);
	mxuvc_video_get_framerate(ch, &info->framerate);
	mxuvc_video_get_resolution(ch, &info->width, &info->height);
	
	if (info->format == VID_FORMAT_H264_RAW || info->format == VID_FORMAT_H264_TS) {
		mxuvc_video_get_bitrate(ch, &info->bitrate);
		mxuvc_video_get_goplen(ch, &info->goplen);
		mxuvc_video_get_profile(ch, &info->profile);
	} else if (info->format == VID_FORMAT_MJPEG_RAW) {
		mxuvc_video_get_compression_quality(ch, &info->compression_quality);
		info->bitrate = 0;
	}

	return 0;
}

int mxuvc_video_get_resolution(video_channel_t ch, uint16_t *width, uint16_t *height)
{

	int ret, res;
	struct video_settings *vset;
	struct video_cfg *vcfg;

	TRACE("Getting resolution on %s channel.\n", chan2str(ch));

	vset = get_vset(ch);
	vcfg = get_vcfg(ch);
	CHECK_ERROR(vcfg->enabled == 0, -1,
			"Unable to get resolution. %s channel is not "
			"enabled.", chan2str(ch));

	switch(vset->cur_video_format) {
	/* For H264 formats, we use the extension to change resolution */
	case VID_FORMAT_H264_RAW:
	case VID_FORMAT_H264_TS:
	case VID_FORMAT_H264_AAC_TS:
		ret = get_ctrl(ch, CTRL_RESOLUTION, GET_CUR, (void*) &res);
		CHECK_ERROR(ret<0, -1, "Failed to set resolution.");
		*width	= (uint16_t) ((res>>16) & 0xffff);
		*height = (uint16_t) (res & 0xffff);
		break;
	/* For the rest ... */
	default:
		*width = vset->cur_frame->width;
		*height = vset->cur_frame->height;
		break;
	}

	return 0;
}



#define DECLARE_STANDARD_SET(control, ctrl_id, size_type) \
	int mxuvc_video_set_##control (video_channel_t ch, size_type value)\
	{\
	RECORD("%s, %i", chan2str(ch), value);\
	int ret;\
	VIDEO_CTRL *ctrl;\
	struct video_cfg *vcfg;\
	\
	TRACE("Setting " #control " to %i on channel %s\n", value,\
			chan2str(ch));\
	\
	vcfg = get_vcfg(ch);\
	CHECK_ERROR(vcfg->enabled == 0, -1,\
			"Unable to set the " #control " on %s channel: the "\
			"channel is not enabled.", chan2str(ch));\
	\
	ctrl = get_ctrl_by_id(ctrl_id);\
	CHECK_ERROR(ctrl->settings[ch].enabled == 0, -1,\
			"Unable to set the " #control " on %s channel: the "\
			"control is not enabled.", chan2str(ch));\
	\
	CHECK_ERROR(value < ctrl->settings[ch].min \
			|| value > ctrl->settings[ch].max, -1,\
			"Value must be in [%i; %i] range for " #control "",\
			(int) ctrl->settings[ch].min, \
			(int) ctrl->settings[ch].max);\
	\
	ret = set_ctrl(ch, ctrl_id, SET_CUR, (void*) &value);\
	CHECK_ERROR(ret < 0, -1, "Unable to change the " #control " on %s "\
			"channel.", chan2str(ch));\
	\
	return 0;\
	}
#define DECLARE_STANDARD_GET(control, ctrl_id, size_type) \
	int mxuvc_video_get_##control(video_channel_t ch, size_type *value)\
	{\
	RECORD("%s, %p", chan2str(ch), value);\
	int ret;\
	VIDEO_CTRL *ctrl;\
	struct video_cfg *vcfg;\
	\
	TRACE("Getting " #control " on %s channel.\n", chan2str(ch));\
	\
	vcfg = get_vcfg(ch);\
	CHECK_ERROR(vcfg->enabled == 0, -1, \
			"Unable to get the " #control " on %s channel: the "\
			"channel is not enabled.", chan2str(ch));\
	\
	ctrl = get_ctrl_by_id(ctrl_id); \
	CHECK_ERROR(ctrl->settings[ch].enabled == 0, -1,\
			"Unable to set the " #control " on %s channel: the "\
			"control is not enabled.", chan2str(ch));\
	\
	ret = get_ctrl(ch, ctrl_id, GET_CUR, (void*) value);\
	CHECK_ERROR(ret < 0, -1, "Unable to get the " #control " on "\
			"%s channel.", chan2str(ch));\
	\
	return 0;\
	}
#define DECLARE_STANDARD(ctrl, ctrl_id, size_type) \
	DECLARE_STANDARD_SET(ctrl, ctrl_id, size_type); \
	DECLARE_STANDARD_GET(ctrl, ctrl_id, size_type);


#define DECLARE_EXT_SET(control, ctrl_id, size_type) \
	int mxuvc_video_set_##control(video_channel_t ch, size_type value)\
	{\
	RECORD("%s, %i", chan2str(ch), value);\
	int ret;\
	VIDEO_CTRL *ctrl;\
	struct video_settings *vset;\
	struct video_cfg *vcfg;\
	\
	TRACE("Setting " #control " to %i on %s channel.\n", value,\
			chan2str(ch));\
	\
	vset = get_vset(ch);\
	vcfg = get_vcfg(ch);\
	CHECK_ERROR(vcfg->enabled == 0, -1, \
			"Unable to set the " #control " on %s channel: the "\
			"channel is not enabled.", chan2str(ch));\
	\
	ctrl = get_ctrl_by_id(ctrl_id);\
	CHECK_ERROR(ctrl->settings[ch].enabled == 0, -1,\
			"Unable to set the " #control " on %s channel: the "\
			"control is not enabled.", chan2str(ch));\
	\
	switch(vset->cur_video_format) {\
	case VID_FORMAT_H264_RAW:\
	case VID_FORMAT_H264_TS:\
	case VID_FORMAT_H264_AAC_TS:\
		ret = set_ctrl(ch, ctrl_id, SET_CUR, (void*) &value);\
		CHECK_ERROR(ret < 0, -1, "Unable to change the " #control " on "\
				"%s channel.", chan2str(ch));\
		break;\
	default:\
		ERROR(-1, #control " cannot be changed for the current format "\
				"(%i) on %s channel", vset->cur_video_format,\
				chan2str(ch));\
	}\
	\
	return 0;\
	}
#define DECLARE_EXT_GET(control, ctrl_id, size_type) \
	int mxuvc_video_get_##control(video_channel_t ch, size_type *value)\
	{\
	RECORD("%s, %p", chan2str(ch), value);\
	int ret;\
	VIDEO_CTRL *ctrl;\
	struct video_settings *vset;\
	struct video_cfg *vcfg;\
	\
	TRACE("Getting " #control " on %s channel.\n", chan2str(ch));\
	\
	vset = get_vset(ch);\
	vcfg = get_vcfg(ch);\
	CHECK_ERROR(vcfg->enabled == 0, -1, \
			"Unable to get the " #control " on %s channel: the "\
			"channel is not enabled.", chan2str(ch));\
	\
	ctrl = get_ctrl_by_id(ctrl_id); \
	CHECK_ERROR(ctrl->settings[ch].enabled == 0, -1,\
			"Unable to set the " #control " on %s channel: the "\
			"control is not enabled.", chan2str(ch));\
	\
	switch(vset->cur_video_format) {\
	case VID_FORMAT_H264_RAW:\
	case VID_FORMAT_H264_TS:\
	case VID_FORMAT_H264_AAC_TS:\
		ret = get_ctrl(ch, ctrl_id, GET_CUR, (void*) value);\
		CHECK_ERROR(ret < 0, -1, "Unable to get the " #control " on "\
				"%s channel.", chan2str(ch));\
		break;\
	default:\
		ERROR(-1, "Cannot get " #control " for the current format "\
				"(%i) on %s channel", vset->cur_video_format,\
				chan2str(ch));\
	}\
	\
	return 0;\
	}
#define DECLARE_EXT(ctrl, ctrl_id, size_type) \
	DECLARE_EXT_SET(ctrl, ctrl_id, size_type); \
	DECLARE_EXT_GET(ctrl, ctrl_id, size_type);

DECLARE_STANDARD(contrast,   CTRL_CONTRAST,       uint16_t);
DECLARE_STANDARD(saturation, CTRL_SATURATION,     uint16_t);
DECLARE_STANDARD(flip_vertical,   CTRL_VFLIP,     video_flip_t);
DECLARE_STANDARD(flip_horizontal, CTRL_HFLIP,     video_flip_t);

DECLARE_EXT(bitrate,         CTRL_BITRATE,        uint32_t);
DECLARE_EXT(goplen,          CTRL_GOPLEN,         uint32_t);
DECLARE_EXT(profile,         CTRL_PROFILE,        video_profile_t);
DECLARE_EXT(maxnal,          CTRL_MAXNAL,         uint32_t);

//CURRENTLY NOT SUPPORTED - This is just place holders to implement the following APIs in future
DECLARE_STANDARD(brightness, CTRL_BRIGHTNESS,     int16_t);
DECLARE_STANDARD(hue,        CTRL_HUE,            int16_t);
DECLARE_STANDARD(gain,       CTRL_GAIN,           uint16_t);
DECLARE_STANDARD(zoom,       CTRL_DIGMULT,        uint16_t);
DECLARE_STANDARD(gamma,      CTRL_GAMMA,          uint16_t);
DECLARE_STANDARD(sharpness,  CTRL_SHARPNESS,      uint16_t);

DECLARE_STANDARD(max_framesize,   CTRL_FRAMESIZE,      uint32_t);
DECLARE_STANDARD(max_analog_gain, CTRL_MAX_ANALOG_GAIN,uint32_t);
DECLARE_STANDARD(histogram_eq,    CTRL_HISTO_EQ,       histo_eq_t);
DECLARE_STANDARD(sharpen_filter,  CTRL_SHARPEN_FILTER, uint32_t);
DECLARE_STANDARD(gain_multiplier, CTRL_GAIN_MULTIPLIER,uint32_t);


int mxuvc_video_cb_buf_done(video_channel_t ch, int buf_index)
{
	//TBD
	return 0;
}

int mxuvc_video_set_dewarp_params(video_channel_t ch, int panel, dewarp_mode_t mode, dewarp_params_t* params)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}

int mxuvc_video_get_dewarp_params(video_channel_t ch, int panel, dewarp_mode_t* mode, dewarp_params_t* params)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}


int mxuvc_audio_get_bitrate(uint32_t *value)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}


int mxuvc_video_set_compression_quality(video_channel_t ch, uint32_t value)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}

int mxuvc_video_get_compression_quality(video_channel_t ch, uint32_t *value)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}

int mxuvc_video_set_avc_level(video_channel_t ch, uint32_t value)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}

int mxuvc_video_get_avc_level(video_channel_t ch, uint32_t *value)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}

int mxuvc_video_set_compositor_params(video_channel_t ch, int panel, panel_mode_t mode, panel_params_t* params)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}


int mxuvc_video_get_compositor_params(video_channel_t ch, int panel, panel_mode_t* mode, panel_params_t* params)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}

int mxuvc_video_get_config_params(video_channel_t ch, config_params_t* params)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}

int mxuvc_video_set_sensor_framerate(video_channel_t ch, uint32_t framerate)
{
    RECORD("%s, %i", chan2str(ch), framerate);
    int ret;

    TRACE("Setting the sensor framerate to %i on %s channel.\n", framerate,
          chan2str(ch));

    struct video_cfg *vcfg;
    vcfg = get_vcfg(ch);
    CHECK_ERROR(vcfg->enabled == 0, -1,
                "Unable to set the sensor framerate on %s channel: "
                "the channel is not enabled.", chan2str(ch));

    VIDEO_CTRL *ctrl;
    ctrl = get_ctrl_by_id(CTRL_SENSOR_FRAMERATE);
    CHECK_ERROR(ctrl->settings[ch].enabled == 0, -1,
			"Unable to set the sensor framerate on %s channel: the "
			"control is not enabled.", chan2str(ch));

    ret = set_ctrl(ch, CTRL_SENSOR_FRAMERATE, SET_CUR, (void*) &framerate);
    CHECK_ERROR(ret < 0, -1, "Unable to set the sensor framerate (%i).", ret);

    return 0;
}

int mxuvc_video_get_sensor_framerate(video_channel_t ch, uint32_t *framerate)
{
    RECORD("%s, %p", chan2str(ch), framerate);
    int ret;

    TRACE("Getting the sensor framerate on %s channel.\n", chan2str(ch));

    struct video_cfg *vcfg;
    vcfg = get_vcfg(ch);
    CHECK_ERROR(vcfg->enabled == 0, -1,
                "Unable to get the sensor framerate on %s channel: "
                "the channel is not enabled.", chan2str(ch));

    VIDEO_CTRL *ctrl;
    ctrl = get_ctrl_by_id(CTRL_SENSOR_FRAMERATE); 
    CHECK_ERROR(ctrl->settings[ch].enabled == 0, -1,
		"Unable to set the sensor framerate on %s channel: the "
		"control is not enabled.", chan2str(ch));

    ret = get_ctrl(ch, CTRL_SENSOR_FRAMERATE, GET_CUR, (void*) framerate);
    CHECK_ERROR(ret < 0, -1, "Unable to get the sensor framerate (%i).", ret);

    return 0;
}

int mxuvc_video_set_tsvc_level(video_channel_t ch, uint32_t value)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}

int mxuvc_video_get_tsvc_level(video_channel_t ch, uint32_t *value)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}

int mxuvc_video_set_vbr_params(video_channel_t ch, vbr_params_t *params)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}

int mxuvc_video_get_vbr_params(video_channel_t ch, vbr_params_t *params)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}

int mxuvc_video_set_crop(video_channel_t ch, crop_info_t *info)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}

int mxuvc_video_get_crop(video_channel_t ch, crop_info_t *info)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}

/*************************************************************************************************/
/* Below ISP functionalities are supported only from ISP tool and not from MXUVC*/ 
/*************************************************************************************************/

int mxuvc_video_set_wb(video_channel_t ch, awb_params_t *params)
{
	// NOT SUPPORTED
	return -1;
}

int mxuvc_video_set_sensor_gain(video_channel_t ch, unsigned int value)
{
	//NOT SUPPORTED
	return -1;
}

int mxuvc_video_set_zone_wb(video_channel_t ch, zone_wb_set_t sel, uint16_t value)
{
	//NOT SUPPORTED
	return -1;
}

int mxuvc_video_get_zone_wb(video_channel_t ch, zone_wb_set_t *sel, uint16_t *value)
{
	//NOT SUPPORTED
	return -1;
}

int mxuvc_video_set_nf(video_channel_t ch, noise_filter_mode_t sel, uint16_t value)
{
	// NOT SUPPORTED
	return -1;
}

int mxuvc_video_get_nf(video_channel_t ch, noise_filter_mode_t *sel, uint16_t *value)
{
	// NOT SUPPORTED
	return -1;
}

int mxuvc_video_set_sensor_exposure(video_channel_t ch, uint16_t value)
{
	// NOT SUPPORTED
	return -1;
}

int mxuvc_video_set_saturation_mode(video_channel_t ch, saturation_mode_t mode)
{
	//NOT SUPPORTED
	return -1;
}

int mxuvc_video_get_saturation_mode(video_channel_t ch, saturation_mode_t *mode)
{
	//NOT SUPPORTED
	return -1;
}

int mxuvc_video_set_brightness_mode(video_channel_t ch, brightness_mode_t mode)
{
	// NOT SUPPORTED
	return -1;
}

int mxuvc_video_get_brightness_mode(video_channel_t ch, brightness_mode_t *mode)
{
	//NOT SUPPORTED
	return -1;
}

int mxuvc_video_set_contrast_mode(video_channel_t ch, contrast_mode_t mode)
{
	// NOT SUPPORTED
	return -1;
}

int mxuvc_video_get_contrast_mode(video_channel_t ch, contrast_mode_t *mode)
{
	// NOT SUPPORTED
	return -1;
}

int mxuvc_video_set_sinter(video_channel_t ch, sinter_info_t *info)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}

int mxuvc_video_get_sinter(video_channel_t ch, sinter_info_t *info)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}

int mxuvc_video_set_isp_roi(video_channel_t ch, isp_ae_roi_info_t *info)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}

int mxuvc_video_get_isp_roi(video_channel_t ch, isp_ae_roi_info_t *info)
{
	//CURRENTLY NOT SUPPORTED
	return -1;
}


