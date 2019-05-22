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

FILE *fd_audio[NUM_VID_FORMAT];

static void audio_cb(unsigned char *buffer, unsigned int size,
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
	for(i=0; i<NUM_AUD_FORMAT; i++) {
		if(fd_audio[i])
			fclose(fd_audio[i]);
	}
}

#define VID "0b6a"
#define PID "4d52"
#define BOARD_SAMPLE_RATE 48000
int main(int argc, char **argv)
{
	int ret=0;
	int count=0;
	int aec_enable = 0;

	if(strncmp(AUDIO_BACKEND, "libusb-uac", 10) == 0) {
		ret = mxuvc_audio_init("libusb-uac", "\
			vid = " VID ";\
			pid = " PID ";\
			packets_per_transfer = 30;\
			num_transfers = 20");
	} else if (strncmp(AUDIO_BACKEND, "alsa", 4) == 0) {
		ret = mxuvc_audio_init("alsa","device = Condor;\
			audio_sampling_rate = 16000");
	} else {
		printf("No compatible audio backend found");
		return 1;
	}
	if(ret < 0)
		return 1;
	/* Set audio settings*/
        mxuvc_alert_init();
        if(ret < 0)
                goto error;
	/* Set audio settings*/
	ret = mxuvc_audio_set_samplerate(AUD_CH1, 16000);
	if(ret < 0)
		goto error;
	ret = mxuvc_audio_register_cb(AUD_CH1, (mxuvc_audio_cb_t)audio_cb, NULL);
	if(ret < 0)
		goto error;

	mxuvc_custom_control_init();

#if 0
	audclk_mode_t mode = 0;

        ret = mxuvc_custom_control_get_audclk_mode(AUD_CH2, &mode);
        if(ret < 0){
               printf("Error 1 \n");
               goto error;
        }
        printf("prev mode= %s \n", (mode==0)? "INTERNAL":"EXTERNAL");

        mode = INTERNAL;
        if (argc > 3)
               mode = atoi(argv[3]);

        printf("new set mode= %s \n", (mode==0)? "INTERNAL":"EXTERNAL");

        ret = mxuvc_custom_control_set_audclk_mode(AUD_CH2, mode);
        if(ret < 0){
                printf("Error 2 \n");
                goto error;
        }

	if( mode == EXTERNAL ){
	    printf("Skip Audio capture... \n");
	    return 0;
        }
#endif

#if 1 
	ret = mxuvc_custom_control_set_audio_codec_samplerate(BOARD_SAMPLE_RATE);
	if(ret < 0)
		goto error;
#endif

	mxuvc_audio_set_volume(100);

	if (argc > 1)
		aec_enable = atoi(argv[1]);
	if(aec_enable)
	{
		ret = mxuvc_custom_control_enable_aec();
		if(ret < 0)
			goto error;
	}
	else
	{
		ret = mxuvc_custom_control_disable_aec();
		if(ret < 0)
			goto error;
	}

	/* Start streaming */
	ret = mxuvc_audio_start(AUD_CH1);
	if(ret < 0)
		goto error;


	/* Main 'loop' */
	int counter;
	if (argc > 2){
		counter = atoi(argv[2]);
	} else
		counter = 15;

	printf("wait %d\n",mxuvc_audio_alive());
	while(counter--) {
		if (!mxuvc_audio_alive())
			goto error;
		printf("\r%i secs left", counter+1);
		fflush(stdout);
		sleep(1);

		/* uncomment to test Mute/Unmute */
		/*	if (counter >= 15)
			mxuvc_set_mic_mute(1);
		else
			mxuvc_set_mic_mute(0);*/
	}
	
	/* Stop audio streaming */
	ret = mxuvc_audio_stop(AUD_CH1);
	if(ret < 0)
		goto error;

	/* Deinitialize and exit */
	mxuvc_audio_deinit();

	close_fds();

	printf("Success\n");

	return 0;
error:
	mxuvc_audio_deinit();
	close_fds();
	printf("Failure\n");
	return 1;
}
