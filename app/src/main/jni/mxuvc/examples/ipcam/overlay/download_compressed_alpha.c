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
	int count=0;
	video_format_t fmt;
	int ch_count = 0, channel;
	int width, height, xoff=0, yoff=0, idx=0;
	video_channel_info_t info;

	char adata;
	char mdata;
	char *file = NULL;

	if ( argc < 2 )
	{
    	printf("USAGE: sudo overlay_compressed_alpha <compressed_alpha_file>\n");
    	return 0;
    }


	//initialize video part **************************
	ret = mxuvc_video_init("v4l2","dev_offset=0");

	if(ret < 0)
		return 1;
	/* Main 'loop' */
	if (argc > 1){
		file = (argv[1]);	
	}
	
	ret = mxuvc_overlay_init();
	if(ret < 0)
	  return 1;

	//get channel count of MUX channel
	ret = mxuvc_video_get_channel_count(&ch_count);
	printf("Total Channel count: %d\n",ch_count);

	ret = mxuvc_download_compressed_alphamap(CH1, file);
	if(ret < 0)
		goto error;

	ret = mxuvc_overlay_deinit();	
	if(ret < 0)
		goto error;

	/* Deinitialize and exit */
	mxuvc_video_deinit();
	
	close_fds();
	
	return 0;

error:
	mxuvc_video_deinit();

	close_fds();	

	printf("Failed\n");
	return 1;
}
