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

//uvc related globals
FILE *fd[NUM_MUX_VID_CHANNELS][NUM_VID_FORMAT];

//uvc callback
static void ch_cb(unsigned char *buffer, unsigned int size,
		video_info_t info, void *user_data)
{
	static const char *basename = "out.chX";
	static const char *ext[NUM_VID_FORMAT] = {
			[VID_FORMAT_H264_RAW]    = ".h264",
			[VID_FORMAT_H264_TS]     = ".ts",
			[VID_FORMAT_H264_AAC_TS] = ".ts",
			[VID_FORMAT_MUX]         = ".mux",
			[VID_FORMAT_MJPEG_RAW]   = ".mjpeg",
	};

	video_format_t fmt = (video_format_t) info.format;
	video_channel_t ch = (video_channel_t) user_data;

	if(fmt < FIRST_VID_FORMAT || fmt >= NUM_VID_FORMAT) {
		printf("Unknown Video format\n");
		return;
	}

	if(fd[ch][fmt] == NULL) {
		char *fname = malloc(strlen(basename) + strlen(ext[fmt]) + 1);
		strcpy(fname, basename);
		strcpy(fname + strlen(fname), ext[fmt]);
		fname[6] = ((char) ch + 1) % 10 + 48;
		fd[ch][fmt] = fopen(fname, "w");
	}


	fwrite(buffer, size, 1, fd[ch][fmt]);
	mxuvc_video_cb_buf_done(ch, info.buf_index);
}

void print_format(video_format_t fmt){
	switch(fmt){
	case VID_FORMAT_H264_RAW:
		printf("Format: H264 Elementary\n");
	break;
	case VID_FORMAT_H264_TS:
		printf("Format: H264 TS\n");
	break;
	case VID_FORMAT_MJPEG_RAW:
		printf("Format: MJPEG\n");
	break;
	case VID_FORMAT_YUY2_RAW:
		printf("Format: YUY2\n");
	break;
	case VID_FORMAT_NV12_RAW:
		printf("Format: NV12\n");
	break;
	case VID_FORMAT_H264_AAC_TS:
		printf("Format: H264 AAC TS\n");
	break;
	case VID_FORMAT_MUX:
		printf("Format: MUX\n");
	break;	
	default:
		printf("unsupported format\n");
	}
}


static void close_fds() {
	int i, j;

	for(i=0; i<NUM_MUX_VID_CHANNELS; i++) {
		for(j=0; j<NUM_VID_FORMAT; j++) {
			if(fd[i][j])
				fclose(fd[i][j]);
		}
	}
}

int main(int argc, char **argv)
{
	int ret;
	int counter=10;
	int count=0;
	video_format_t fmt;
	int ch_count = 0, channel;
	int width, height, xoff=0, yoff=0, idx=0;
	video_channel_info_t info;

	char adata;
	char mdata;
	char *file = NULL;

	if ( argc < 3 )
	{
    	printf("USAGE: sudo overlay_privacy [<maskwidth>] [<maskheight>] [<xoffset>(optional | default 0)] [<yoffset>(optional | default 0)] [<index>(optional | default 0)] [<timeout>(optional | default 10)]\n");
    	return 0;
    }


	//initialize video part **************************
	ret = mxuvc_video_init("v4l2","dev_offset=0");

	if(ret < 0)
		return 1;
	/* Main 'loop' */
	if (argc > 1){
		width = atoi(argv[1]);	
	} 

	if (argc > 2){
		height = atoi(argv[2]);
	}

	if (argc > 3){
		xoff = atoi(argv[3]);
	}
	else {
		xoff = 0;
	}

	if (argc > 4){
		yoff = atoi(argv[4]);
	}
	else {
		yoff = 0;
	}

	if (argc > 5){
		idx = atoi(argv[5]);
	}
	else {
		idx = 0;
	}

	if (argc > 6){
		counter = atoi(argv[6]);
	}
	else {
		counter = 10;
	}

	if(counter < 0){
	printf("counter should be +ive number\n");		
	return 1;
	}
	

	ret = mxuvc_overlay_init();
	if(ret < 0)
	  return 1;
	printf("overlay init done\n");
	

	/* Register callback functions */
	ret = mxuvc_video_register_cb(CH1, ch_cb, (void*)CH1);
	if(ret < 0)
		goto error;
	
	printf("\n");
	//get channel count of MUX channel
	ret = mxuvc_video_get_channel_count(&ch_count);
	printf("Total Channel count: %d\n",ch_count);
	//remove raw channel from count
	ch_count = ch_count - 1;

	for(channel=CH1 ; channel<ch_count; channel++)
	{
		ret = mxuvc_video_register_cb(channel, ch_cb, (void*)channel);
		if(ret < 0)
			goto error;
	}

	privacy_color_t color;
	privacy_params_t params;

	params.color = &color;
	//YUVA format
	//A = 0xFF = 0% transparent.
	//A = 0x00 = 100% transparent. No mask will be displayed.
	
	params.color->yuva = 0x008080FF; // black. 0% transparent
	// params.color->yuva = 0x4C54FF7F; // Red. 50% transparent

	#ifdef USE_RECT
	params.type = PRIVACY_SHAPE_RECT;
	privacy_mask_shape_rect_t rect;

	rect.xoff = xoff;
	rect.yoff = yoff;
	rect.width = width;
	rect.height = height;

	params.shape = (privacy_shape_t*)&rect;
	#else
	params.type = PRIVACY_SHAPE_POLYGON;
	privacy_mask_shape_polygon_t polygon;
			
	//set of points starting with point nearest to origin (0, 0) and clockwise.
	privacy_mask_point_t points[] = {{xoff, yoff}, {xoff+width, yoff}, {xoff+width, yoff+height}, {xoff, yoff+height}}; 
	polygon.num_points = sizeof(points)/sizeof(privacy_mask_point_t);
	polygon.points = &points[0];
	params.shape = (privacy_shape_t*)&polygon;
	#endif
	ret = mxuvc_overlay_privacy_add_mask(CH1, &params, idx);
	if(ret < 0)
		goto error;

	/***** Start video streaming ************/
	for(channel=CH1; channel<ch_count ; channel++){
		/* Start streaming */
		ret = mxuvc_video_start(channel);
		if(ret < 0)
			goto error;
	}

	//sleep
	usleep(5000);
	
	
	while(counter--) {
		if (!mxuvc_video_alive())
			goto error;
		printf("%i secs left\n", counter+1);
		if(counter-2 <= 0)
		{
			ret = mxuvc_overlay_privacy_remove_mask(CH1, idx); 
			if(ret < 0)
				goto error;
		}

		fflush(stdout);
		sleep(1);
	}	

	ret = mxuvc_overlay_deinit();	
	if(ret < 0)
		goto error;

	/* Stop video streaming */
	for(channel=CH1; channel<ch_count ; channel++)
	{	
		/* Stop streaming */
		ret = mxuvc_video_stop(channel);
		if(ret < 0)
			goto error;
	}
	/* Deinitialize and exit */
	mxuvc_video_deinit();
	
	close_fds();
	
	return 0;

error:
	//mxuvc_audio_deinit();
	mxuvc_video_deinit();

	close_fds();	

	printf("Failed\n");
	return 1;
}
