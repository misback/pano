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

#include "common.h"
extern int ipcam_mode;

const char* chan2str(video_channel_t ch)
{
	if (ipcam_mode){
		static const char *mapping[NUM_IPCAM_VID_CHANNELS] = {
			[CH1] = "CH1",
			[CH2] = "CH2",
			[CH3] = "CH3",
			[CH4] = "CH4",
			[CH5] = "CH5",
			[CH6] = "CH6",
			[CH7] = "CH7",
			[CH_RAW] = "CH_RAW",
		};
		if (ch < NUM_IPCAM_VID_CHANNELS)
			return mapping[ch];
		else
			return "Unknown video channel";
	} else {
		static const char *mapping[NUM_VID_CHANNEL] = {
			[CH_MAIN]    = "CH_MAIN",
			[CH_PREVIEW] = "CH_PREVIEW"
		};
		if (ch < NUM_VID_CHANNEL)
			return mapping[ch];
		else
			return "Unknown video channel";
	}
}

const char* vidformat2str(video_format_t format)
{
	static const char *mapping[NUM_VID_FORMAT] = {
		[VID_FORMAT_H264_RAW]    = "VID_FORMAT_H264_RAW",
		[VID_FORMAT_H264_TS]     = "VID_FORMAT_H264_TS",
		[VID_FORMAT_MJPEG_RAW]   = "VID_FORMAT_MJPEG_RAW",
		[VID_FORMAT_YUY2_RAW]    = "VID_FORMAT_YUY2_RAW",
		[VID_FORMAT_NV12_RAW]    = "VID_FORMAT_NV12_RAW",
		[VID_FORMAT_H264_AAC_TS] = "VID_FORMAT_H264_AAC_TS",
		[VID_FORMAT_GREY_RAW]    = "VID_FORMAT_GREY_RAW",
		[VID_FORMAT_MUX]	 = "VID_FORMAT_MUX",
	};
	if (format < NUM_VID_FORMAT)
		return mapping[format];
	else
		return "Unknown video format";
}
const char* profile2str(video_profile_t profile)
{
	static const char *mapping[NUM_PROFILE] = {
		[PROFILE_BASELINE]   = "Baseline",
		[PROFILE_MAIN]       = "Main",
		[PROFILE_HIGH]       = "High",
	};
	if (profile < NUM_PROFILE)
		return mapping[profile];
	else
		return "Unknown profile";
}

const char* audformat2str(audio_format_t format)
{
	static const char *mapping[NUM_AUD_FORMAT] = {
		[AUD_FORMAT_PCM_RAW]    = "AUD_FORMAT_PCM_RAW",
		[AUD_FORMAT_AAC_RAW]    = "AUD_FORMAT_AAC_RAW",
		[AUD_FORMAT_OPUS_RAW]   = "AUD_FORMAT_OPUS_RAW",
	};
	if (format < NUM_AUD_FORMAT)
		return mapping[format];
	else
		return "Unknown audio format";
}

static char* remove_trailing_whitespace(char *str) 
{
	int len;

	while(*str != '\0') {
		if(*str !=' ' && *str != '\t')
			break;
		str++;
	}
	len = strlen(str);
	while(len-->0) {
		if(str[len] != ' ' && str[len] != '\t')
			break;
		str[len] = '\0';
	}

	return str;
}

int next_opt(char *str, char **opt, char **value)
{
	static char *token, *subtoken;
	static char *saveptr1, *saveptr2;

	token = strtok_r(str, ";", &saveptr1);
	if (token == NULL) {
		*opt = NULL;
		*value = NULL;
		return -1;
	}

	subtoken = strtok_r(token, "=", &saveptr2);
	if (subtoken == NULL) {
		*opt = NULL;
		*value = NULL;
		return -1;
	}
	*opt = remove_trailing_whitespace(subtoken);

	subtoken = strtok_r(NULL, "=", &saveptr2);
	if (subtoken == NULL) {
		*opt = NULL;
		*value = NULL;
		return -1;
	}
	*value = remove_trailing_whitespace(subtoken);

	return 0;
}
