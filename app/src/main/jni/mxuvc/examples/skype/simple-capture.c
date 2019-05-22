/*******************************************************************************
*
* The content of this file or document is CONFIDENTIAL and PROPRIETARY
* to Maxim Integrated Products.  It is subject to the terms of a
* License Agreement between Licensee and Maxim Integrated Products.
* restricting among other things, the use, reproduction, distribution
* and transfer.  Each of the embodiments, including this information and
* any derivative work shall retain this copyright notice.
*
* Copyright (c) 2011 Maxim Integrated Products.
* All rights reserved.
*
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <pthread.h>

#include "mxuvc.h"

FILE *fd_main[NUM_VID_FORMAT];
FILE *fd_preview[NUM_VID_FORMAT];
FILE *fd_audio[NUM_VID_FORMAT];

static void main_cb(unsigned char *buffer, unsigned int size,
		video_info_t info, void *user_data)
{

	video_format_t fmt = (video_format_t) info.format;

	switch(fmt) {
	case VID_FORMAT_H264_RAW:
		if(fd_main[fmt] == NULL)
			fd_main[fmt] = fopen("out.main.h264", "w");
		break;
	case VID_FORMAT_H264_TS:
	case VID_FORMAT_H264_AAC_TS:
		if(fd_main[fmt] == NULL)
			fd_main[fmt] = fopen("out.main.ts", "w");
		break;
	default:
		printf("Video format not supported\n");
		return;
	}

	fwrite(buffer, size, 1, fd_main[fmt]);
}
static void preview_cb(unsigned char *buffer, unsigned int size,
		video_info_t info, void *user_data)
{
	video_format_t fmt = (video_format_t) info.format;
	switch(fmt) {
	case VID_FORMAT_H264_RAW:
		if(fd_preview[fmt] == NULL)
			fd_preview[fmt] = fopen("out.preview.h264", "w");
		break;
	case VID_FORMAT_H264_TS:
	case VID_FORMAT_H264_AAC_TS:
		if(fd_preview[fmt] == NULL)
			fd_preview[fmt] = fopen("out.preview.ts", "w");
		break;
	case VID_FORMAT_MJPEG_RAW:
		if(fd_preview[fmt] == NULL)
			fd_preview[fmt] = fopen("out.preview.mjpg", "w");
		break;
	case VID_FORMAT_YUY2_RAW:
		if(fd_preview[fmt] == NULL)
			fd_preview[fmt] = fopen("out.preview.yuy2", "w");
		break;
	case VID_FORMAT_NV12_RAW:
		if(fd_preview[fmt] == NULL)
			fd_preview[fmt] = fopen("out.preview.nv12", "w");
		break;
	case VID_FORMAT_GREY_RAW:
		if(fd_preview[fmt] == NULL)
			fd_preview[fmt] = fopen("out.preview.y8", "w");
		break;
	default:
		printf("Video format not supported\n");
		return;
	}

	fwrite(buffer, size, 1, fd_preview[fmt]);
}

static int audio_cb(unsigned char *buffer, unsigned int size,
		int format, uint64_t ts, void *user_data, audio_params_t *param)
{
	audio_format_t fmt = (audio_format_t) format;

	switch(fmt) {
	case AUD_FORMAT_PCM_RAW:
		if(fd_audio[fmt] == NULL)
			fd_audio[fmt] = fopen("out.audio.pcm", "w");
		break;
	default:
		printf("Audio format not supported\n");
		return;
	}

	fwrite(buffer, size, 1, fd_audio[fmt]);
}

static void close_fds() {
	int i;
	for(i=0; i<NUM_VID_FORMAT; i++) {
		if(fd_main[i])
			fclose(fd_main[i]);
		if(fd_preview[i])
			fclose(fd_preview[i]);
	}
	for(i=0; i<NUM_AUD_FORMAT; i++) {
		if(fd_audio[i])
			fclose(fd_audio[i]);
	}
}

#define VID "0b6a"
#define PID "4d52"
int main(int argc, char **argv)
{
	int ret=0;
	int count=0;

	/* Initialize camera */
	ret = mxuvc_video_init("v4l2", "dev_offset=0");
	if(ret < 0)
		return 1;

	ret = mxuvc_audio_init("alsa","device = Condor");
	if(ret < 0)
		return 1;

	/* Set audio settings*/
	ret = mxuvc_audio_set_samplerate(AUD_CH1, 24000);
	if(ret < 0)
		goto error;

	/* Set video settings */
	ret = mxuvc_video_set_format(CH_MAIN, VID_FORMAT_H264_RAW);
	if(ret < 0)
		goto error;
	ret = mxuvc_video_set_resolution(CH_MAIN, 1280, 720);
	if(ret < 0)
		goto error;
	ret = mxuvc_video_set_format(CH_PREVIEW, VID_FORMAT_MJPEG_RAW);
	if(ret < 0)
		goto error;
	ret = mxuvc_video_set_resolution(CH_PREVIEW, 640, 480);
	if(ret < 0)
		goto error;

	/* Register callback functions */
	ret = mxuvc_video_register_cb(CH_MAIN, main_cb, NULL);
	if(ret < 0)
		goto error;
	ret = mxuvc_video_register_cb(CH_PREVIEW, preview_cb, NULL);
	if(ret < 0)
		goto error;
	ret = mxuvc_audio_register_cb(AUD_CH1,(mxuvc_audio_cb_t) audio_cb, NULL);
	if(ret < 0)
		goto error;

	/* Start streaming */
	ret = mxuvc_audio_start(AUD_CH1);
	if(ret < 0)
		goto error;
	ret = mxuvc_video_start(CH_MAIN);
	if(ret < 0)
		goto error;
	ret = mxuvc_video_start(CH_PREVIEW);
	if(ret < 0)
		goto error;

	/* In case of Bulk mode: wait a few ms and force an iframe */
	usleep(50000);
	mxuvc_video_force_iframe(CH_MAIN);

	/* Main 'loop' */
	int counter;
	if (argc > 1){
		counter = atoi(argv[1]);
	} else
		counter = 15;

	while(counter--) {
		if( !mxuvc_video_alive() || !mxuvc_audio_alive())
			goto error;
		printf("\r%i secs left", counter+1);
		fflush(stdout);
		sleep(1);
	}
	
	/* Stop audio/video streaming */
	ret = mxuvc_audio_stop(AUD_CH1);
	if(ret < 0)
		goto error;
	ret = mxuvc_video_stop(CH_MAIN);
	if(ret < 0)
		goto error;
	ret = mxuvc_video_stop(CH_PREVIEW);
	if(ret < 0)
		goto error;

	/* Deinitialize and exit */
	mxuvc_video_deinit();
	mxuvc_audio_deinit();

	close_fds();

	printf("Success\n");

	return 0;
error:
	mxuvc_video_deinit();
	mxuvc_audio_deinit();
	close_fds();
	printf("Failure\n");
	return 1;
}
