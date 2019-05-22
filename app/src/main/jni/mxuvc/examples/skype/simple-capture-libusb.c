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
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <pthread.h>

#include "mxuvc.h"

FILE *fd_main[NUM_VID_FORMAT];
FILE *fd_preview[NUM_VID_FORMAT];
FILE *fd_audio[NUM_VID_FORMAT];

static unsigned int frame_cnt_main = 0;
static unsigned int frame_cnt_prvw = 0;

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
	if(size > 100)
	{
		frame_cnt_main++;
	}
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

    frame_cnt_prvw++;
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

	/* Initialize camera */

	ret = mxuvc_video_init("libusb-uvc", "vid=0x29fe;pid=0x4d53;packets_per_transfer=125");
	if(ret < 0)
		return 1;

    /* Set video settings */
	ret = mxuvc_video_set_format(CH_MAIN, VID_FORMAT_H264_RAW);
	if(ret < 0)
		goto error;

	ret = mxuvc_video_set_resolution(CH_MAIN, 1920, 960);
	if(ret < 0)
		goto error;
	ret = mxuvc_video_set_format(CH_PREVIEW, VID_FORMAT_H264_RAW);
	if(ret < 0)
		goto error;
	ret = mxuvc_video_set_resolution(CH_PREVIEW, 1920, 960);
	if(ret < 0)
		goto error;

	/* Register callback functions */
	ret = mxuvc_video_register_cb(CH_MAIN, main_cb, NULL);
	if(ret < 0)
		goto error;
	ret = mxuvc_video_register_cb(CH_PREVIEW, preview_cb, NULL);
	if(ret < 0)
		goto error;

	/* Start streaming */
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
		if( !mxuvc_video_alive())
			goto error;
		printf("\r%i secs left", counter+1); 
		fflush(stdout);
		sleep(1);
	}
	
	/* Stop video streaming */
	ret = mxuvc_video_stop(CH_MAIN);
	if(ret < 0)
		goto error;
	ret = mxuvc_video_stop(CH_PREVIEW);
	if(ret < 0)
		goto error;

	/* Deinitialize and exit */
	mxuvc_video_deinit();

	close_fds();

	printf("Success\n");
    printf("main channel: %d frames captured\n", frame_cnt_main);
    printf("prvw channel: %d frames captured\n", frame_cnt_prvw);

    return 0;
error:
	mxuvc_video_deinit();
	close_fds();
	printf("Failure\n");
	return 1;
}
