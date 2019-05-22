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
	int counter=0;
	int count=0;
	video_format_t fmt;
	int ch_count = 0;
        long channel;
	int width, height, xoff, yoff;
	video_channel_info_t info;

	char adata;
	char mdata;
	char *yuvfile = NULL;
	char *alphafile = NULL;
	if ( argc < 4 )
	{
    	printf("USAGE: sudo overlay_image <path_to_logo_yuv420_file> [<imagewidth>(max 640)] [<imageheight>(max 480)] [<xoffset>(optional | default 0)] [<yoffset>(optional | default 0)] [<path_to_per_pixel_alpha_mask>(optional | default 0)]\n");
    	return 0;
    }


	//initialize video part **************************
	ret = mxuvc_video_init("v4l2","dev_offset=0");

	if(ret < 0)
		return 1;
	/* Main 'loop' */
	if (argc > 1){
		yuvfile = argv[1];
	}

	if (argc > 2){
		width = atoi(argv[2]);	
	} 

	if (argc > 3){
		height = atoi(argv[3]);
	}

	if (argc > 4){
		xoff = atoi(argv[4]);
	}
	else {
		xoff = 0;
	}

	if (argc > 5){
		yoff = atoi(argv[5]);
	}
	else {
		yoff = 0;
	}

	if (argc > 6){
		alphafile = argv[6];
	}
	else {
		alphafile = NULL;
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

	/***** Start video streaming ************/
	for(channel=CH1; channel<ch_count ; channel++){
		/* Start streaming */
		ret = mxuvc_video_start(channel);
		if(ret < 0)
			goto error;
	}

	//sleep
	usleep(5000);
	counter = 20;
	overlay_image_params_t ovimage[8];
	uint8_t alpha = 0xFF;

	while(counter--) {
		if (!mxuvc_video_alive())
			goto error;

		printf("%i secs left\n", counter+1);
		int ch=0;
		if(counter == 18)
		{
              
			ch = CH1;
			// for(ch=CH1; ch<ch_count; ch++)
			// {	
				ovimage[ch].width = width;
				ovimage[ch].height = height;
				ovimage[ch].xoff = xoff;
				ovimage[ch].yoff = yoff;
				ovimage[ch].idx = 0;
				ovimage[ch].alpha = alpha;
				mxuvc_overlay_add_image(ch, &ovimage[ch], yuvfile, alphafile);
			// }
		}


		if(counter == 2)
		{
			ch = CH1;

			// for(ch=CH1; ch < ch_count; ch++)
			ret = mxuvc_overlay_remove_image(ch, ovimage[ch].idx);
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
