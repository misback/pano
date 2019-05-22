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
#include <math.h>
#include <inttypes.h>

#include "mxuvc.h"

//uac related globals
FILE *fd_aud;
#define OBJECTTYPE_AACLC (2)
#define ADTS_HEADER_LEN (7)

static const int SampleFreq[12] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000
};

#define SAMP_FREQ 16000.0
uint64_t sam_interval; 

uint64_t prev_ts = 0;

uint64_t permissible_range = 10; //in percentage

uint64_t upper_sam_interval;
uint64_t lower_sam_interval;

void get_adts_header(audio_params_t *h, unsigned char *adtsHeader)
{
    int i;
    int samplingFreqIndex;

    // clear the adts header
    for (i = 0; i < ADTS_HEADER_LEN; i++)
    {
        adtsHeader[i] = 0;
    }

    // bits 0-11 are sync
    adtsHeader[0] = 0xff;
    adtsHeader[1] = 0xf0;

    // bit 12 is mpeg4 which means clear the bit

    // bits 13-14 is layer, always 00

    // bit 15 is protection absent (no crc)
    adtsHeader[1] |= 0x1;

    // bits 16-17 is profile which is 01 for AAC-LC
    adtsHeader[2] = 0x40;

    // bits 18-21 is sampling frequency index
    for (i=0; i<12; i++)
    {
        if ( SampleFreq[i] == h->samplefreq )
        {
            samplingFreqIndex =  i;
            break;
        }
    }


    adtsHeader[2] |= (samplingFreqIndex << 2);

    // bit 22 is private

    // bit 23-25 is channel config.  However since we are mono or stereo
    // bit 23 is always zero
    adtsHeader[3] = h->channelno << 6;

    // bits 26-27 are original/home and zeroed

    // bits 28-29 are copyright bits and zeroed

    // bits 30-42 is sample length including header len.  First we get qbox length,
    // then sample length and then add header length


    // adjust for headers
    int frameSize = h->framesize + ADTS_HEADER_LEN;

    // get upper 2 bits of 13 bit length and move them to lower 2 bits
    adtsHeader[3] |= (frameSize & 0x1800) >> 11;

    // get middle 8 bits of length
    adtsHeader[4] = (frameSize & 0x7f8) >> 3;

    // get lower 3 bits of length and put as 3 msb
    adtsHeader[5] = (frameSize & 0x7) << 5;

    // bits 43-53 is buffer fulless but we use vbr so 0x7f
    adtsHeader[5] |= 0x1f;
    adtsHeader[6] = 0xfc;

    // bits 54-55 are # of rdb in frame which is always 1 so we write 1 - 1 = 0
    // which means do not write

}

//uac call-back
static void audio_cb(unsigned char *buffer, unsigned int size,
		int format, uint64_t ts, void *user_data, audio_params_t *param)
{
	audio_format_t fmt = (audio_format_t) format;
	unsigned char adtsHeader[7];

	switch(fmt) {
	case AUD_FORMAT_AAC_RAW: //TBD
		if(fd_aud == NULL){
			fd_aud = fopen("out.audio.aac", "w");
			//calculate required sampling interval
			//(1024/sam_freq)*90  //90 is resampler freq
			sam_interval = (uint64_t)(((float)(1024*1000/SAMP_FREQ))*90);
			uint64_t percent = (uint64_t)(sam_interval*permissible_range)/100;
			upper_sam_interval = (uint64_t)(sam_interval + percent);
			lower_sam_interval = (uint64_t)(sam_interval - percent);
			printf("sam_interval %" PRIu64 "upper_limit %" PRIu64 " low_limit %" PRIu64 "\n",
					sam_interval,upper_sam_interval,lower_sam_interval);
		}

		if((((ts-prev_ts) > upper_sam_interval) || ((ts-prev_ts) < lower_sam_interval)) && (prev_ts))
			printf("out of range: %" PRIu64 ", last ts %" PRIu64 " preset ts %" PRIu64 "\n",(ts-prev_ts),prev_ts, ts);

		prev_ts = ts;
			
		if(param->samplefreq != SAMP_FREQ)	
			printf("Wrong sampling freq, expected %fhz received pkt with %dhz\n",SAMP_FREQ,param->samplefreq);

		get_adts_header(param, adtsHeader);
		fwrite(adtsHeader, ADTS_HEADER_LEN, 1, fd_aud);
		break;
	case AUD_FORMAT_PCM_RAW:
		if(fd_aud == NULL)
			fd_aud = fopen("out.audio.pcm", "w");
		break;
	default:
		printf("Audio format not supported\n");
		return;
	}

	fwrite(buffer, size, 1, fd_aud);
}

static void close_fds() {
	int i, j;
	fclose(fd_aud);
}

void aalarm_callback(audio_alert_info *audalert_info, void *user_data)
{
	audio_alert_info *aa_info = (audio_alert_info *)audalert_info;
	int audioIntensityDB = aa_info->audioThresholdDB;

	audioIntensityDB = (audioIntensityDB == 0)? 0 : (20 * log10(audioIntensityDB)) + 0.5;
	
	printf("aalarm_callback  audio alarm Intensity %ddb\n",audioIntensityDB);
	if ( aa_info->state == AUDIOALERT_STARTED )
		printf("Audio alarm event started\n");
	else if ( aa_info->state == AUDIOALERT_STOPPED )
		printf("Audio alarm event stopped\n");

}

int main(int argc, char **argv)
{
	int ret;
	int counter=0;
	int count=0;
	int ch_count = 0, channel;
	uint16_t width, hight;
	int framerate = 0;
	int goplen = 0;
	int audio_threshold = 30;
	int current_audio_intensity = 0;
	char data;

	//initialize audio part **************************
	ret = mxuvc_audio_init("alsa","device = Condor");
	if(ret<0)
		goto error;

	ret = mxuvc_audio_register_cb(AUD_CH2,(mxuvc_audio_cb_t) audio_cb, NULL);
	if(ret < 0)
		goto error;
	
	ret = mxuvc_audio_set_samplerate(AUD_CH2, SAMP_FREQ);
	if(ret < 0)
		goto error;

	mxuvc_audio_set_volume(100);
	
	/***** Start audio  streaming ************/
	ret = mxuvc_audio_start(AUD_CH2);
	if(ret < 0)
		goto error;

	if (audio_threshold > 0){
		//init audio alarm 
		ret = mxuvc_alert_init();
		if(ret < 0)
			goto error;
		ret = mxuvc_alert_audio_set_threshold(audio_threshold);
		if (ret){
		    printf("mxuvc_alert_audio_set_threshold() failed\n");
		    goto error;
		}
		ret = mxuvc_alert_audio_register_cb(&aalarm_callback, (void *)&data);
		if (ret){
			printf("mxuvc_alert_audio_register_callback() failed\n");
			goto error;
		}

		ret = mxuvc_alert_audio_enable();
		if (ret){
			printf("mxuvc_alert_audio_enable() failed\n");
			goto error;
		}

	}
	
	//sleep
	usleep(5000);

	/* Main 'loop' */
	if (argc > 1){
		counter = atoi(argv[1]);
	} else
		counter = 15;

	while(counter--) {
		if (!mxuvc_audio_alive())
			goto error;
		printf("\r%i secs left", counter);

		//get current audio intensity
		ret = mxuvc_get_current_audio_intensity(&current_audio_intensity);
		if(ret){
			printf("mxuvc_get_current_audio_intensity() failed\n");
			goto error;
		}else
			printf("Current audio intensity %ddB\n",current_audio_intensity);

		if(counter == 10){
		    printf(" setting threshold = %d\n",20);
			ret = mxuvc_alert_audio_set_threshold(20);

			if (ret){
				printf("mxuvc_alert_audio_set_threshold() failed\n");
				goto error;
			}
		}

		if(counter == 11){
		    printf(" setting threshold = %d\n",audio_threshold);
			ret = mxuvc_alert_audio_set_threshold(audio_threshold);

			if (ret){
				printf("mxuvc_alert_audio_set_threshold() failed\n");
				goto error;
			}

		}

		fflush(stdout);
		sleep(1);
	}	

	if (audio_threshold > 0){
		mxuvc_alert_audio_disable();
		sleep(1);
	}
	mxuvc_alert_deinit();

	/* Stop audio streaming */
	ret = mxuvc_audio_stop(AUD_CH2);
	if(ret < 0)
		goto error;

	mxuvc_audio_deinit();
	
	close_fds();
	
	return 0;

error:
	mxuvc_audio_deinit();

	close_fds();	

	printf("Failed\n");
	return 1;
}
