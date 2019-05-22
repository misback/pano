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
#include <unistd.h>
#include <pthread.h>

#include "mxuvc.h"
#include "qmed.h"

#define  SNAPSHOT_TIME  3
FILE *fd[NUM_MUX_VID_CHANNELS][NUM_VID_FORMAT];
FILE *fdJPG;
bool bTakeSnapshot[NUM_MUX_VID_CHANNELS]={0};


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
            [VID_FORMAT_METADATAS]   = ".datas",
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
		free(fname);
	}

	if( (fmt==VID_FORMAT_MJPEG_RAW)&&bTakeSnapshot[ch]){
        char *fname = malloc(strlen(basename) + strlen(".jpg") + 1);
        strcpy(fname, basename);
		strcpy(fname + strlen(fname), ".jpg");
		fname[6] = ((char) ch + 1) % 10 + 48;
        fdJPG=fopen(fname, "w");
        if(fdJPG){
            fwrite(buffer, size, 1, fdJPG);
            fclose(fdJPG);
            bTakeSnapshot[ch]=false;
        }
        free(fname);
    }
    if(fmt==VID_FORMAT_H264_RAW) {
        if(info.qmed.total_len != 0) {
            QMedH264Struct *qmedH264;
            int qp;
            qp = GetQMedH264QP(info.qmed.qmedPtr);
            //printf("** QP = %d\n", qp);
        }
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
	int ret=0;
	int count=0;
	int ch_count = 0;
    long channel;
	camer_mode_t mode;

	/* Initialize camera */
	ret = mxuvc_video_init("v4l2","dev_offset=0");

	if(ret < 0)
		return 1;

	ret = mxuvc_get_camera_mode(&mode);
	if(ret < 0)
		return 1;
	printf("Camera mode %s\n",mode==IPCAM ? "IPCAM":"SKYPE");

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

	for(channel=CH2 ; channel<ch_count; channel++)
	{
		ret = mxuvc_video_register_cb(channel, ch_cb, (void*)channel);
		if(ret < 0)
			goto error;
	}

	printf("\n");
	for(channel=CH1; channel<ch_count ; channel++){
		/* Start streaming */
		ret = mxuvc_video_start(channel);
		if(ret < 0)
			goto error;
	}

	usleep(5000);

	/* Main 'loop' */
	int counter, cap_time;
	if (argc > 1){
		counter = atoi(argv[1]);
	} else
		counter = 15;

    cap_time = counter;

	while(counter--) {
		if(!mxuvc_video_alive()) {
			goto error;
		}
        if( (cap_time - counter) == SNAPSHOT_TIME )
                for(channel=CH1; channel<ch_count ; channel++)
                    bTakeSnapshot[channel]=true;

		printf("\r%i secs left\n", counter+1);
		fflush(stdout);
		sleep(1);
	}

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

	printf("Success\n");

	return 0;
error:
	mxuvc_video_deinit();
	close_fds();
	printf("Failure\n");
	return 1;
}
