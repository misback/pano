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

#define SAMP_FREQ 24000.0
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

//uvc related globals
FILE *fd[NUM_MUX_VID_CHANNELS][NUM_VID_FORMAT];


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
			printf("sam_interval %" PRIu64 "upper_limit %" PRIu64 "low_limit %" PRIu64 "\n",
					sam_interval,upper_sam_interval,lower_sam_interval);
		}

		if((((ts-prev_ts) > upper_sam_interval) || ((ts-prev_ts) < lower_sam_interval)) && (prev_ts))
			printf("out of range: %" PRIu64 ", last ts %" PRIu64 "preset ts %" PRIu64 "\n",(ts-prev_ts),prev_ts, ts);

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
		free(fname);
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

void video_testing(int counter)
{
	if (counter == 9){
		mxuvc_video_set_framerate(CH1, 5);
		mxuvc_video_set_framerate(CH2, 5);
		mxuvc_video_set_framerate(CH3, 5);
		uint16_t zoom=0;
		mxuvc_video_get_zoom(CH1, &zoom);
		printf("CH1 zoom = %d\n",zoom);
	}

	if (counter == 6){
		mxuvc_video_set_framerate(CH1, 15);
		mxuvc_video_set_framerate(CH2, 15);
		mxuvc_video_set_framerate(CH3, 15);
		mxuvc_video_set_zoom(CH1, 20);
		mxuvc_video_set_pantilt(CH1, 3600*100, 3600*100);
	}

	if (counter == 3){
		mxuvc_video_set_framerate(CH1, 30);
		mxuvc_video_set_framerate(CH2, 30);
		mxuvc_video_set_framerate(CH3, 30);
		mxuvc_video_set_zoom(CH1, 80);
		mxuvc_video_set_pantilt(CH1, 0, 0);
	}
}

static void close_fds() {
	int i, j;
	fclose(fd_aud);

	for(i=0; i<NUM_MUX_VID_CHANNELS; i++) {
		for(j=0; j<NUM_VID_FORMAT; j++) {
			if(fd[i][j])
				fclose(fd[i][j]);
		}
	}
}

void aalarm_callback(audio_alert_info *audalert_info, void *user_data)
{
        audio_alert_info *aa_info = (audio_alert_info *)audalert_info;
        int audioIntensityDB = aa_info->audioThresholdDB;

        audioIntensityDB = (20 * log10(audioIntensityDB)) + 0.5;

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
	video_format_t fmt;
	int ch_count = 0; 
	long channel;
	uint16_t hight;
	int framerate = 0;
	int goplen = 0;
	video_channel_info_t info;
	int audio_threshold = 70;
	int current_audio_intensity = 0;
	int reg_id;	/* region number; used when alert is generated */
	int x_offt;	/* starting x offset */
	int y_offt;	/* starting y offset */
	int width;	/* width of region */
	int height;	/* height of region */
	char adata;
	char mdata;
	
	//initialize audio part **************************
	ret = mxuvc_audio_init("alsa","device = MAX64380");
	if(ret<0)
		goto error;

	ret = mxuvc_audio_register_cb(AUD_CH2, (mxuvc_audio_cb_t) audio_cb, NULL);
	if(ret < 0)
		goto error;
	
	ret = mxuvc_audio_set_samplerate(AUD_CH2, SAMP_FREQ);
	if(ret < 0)
		goto error;

	mxuvc_audio_set_volume(100);

	//initialize video part **************************
	ret = mxuvc_video_init("v4l2","dev_offset=0");

	if(ret < 0)
		return 1;

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

	/*** enquire every channel capabilities ****/
	for(channel=CH1; channel<ch_count ; channel++)
	{
		mxuvc_video_get_channel_info(channel, &info);
		printf("\nCH%ld Channel Config:\n",channel+1);
		print_format(info.format);
		printf("width %d height %d\n",info.width,info.height);
		printf("framerate %dfps\n",info.framerate);
		
		if(info.format == VID_FORMAT_H264_RAW || 
				info.format == VID_FORMAT_H264_TS)
		{
			printf("gop length %d\n",info.goplen);	
			printf("setting gop length to 30\n");
			ret = mxuvc_video_set_goplen(channel, 30);
			if(ret < 0)
				goto error;
			ret = mxuvc_video_get_goplen(channel, &goplen);
			if(ret < 0)
				goto error;
			printf("gop length %d\n",goplen);
			printf("profile %d\n",info.profile);
			printf("bitrate %d\n",info.bitrate);
		}
	
	}

	printf("\n");
	video_profile_t profile;
	ret = mxuvc_video_get_profile(CH1, &profile);
	if(ret < 0)
		goto error;
	printf("CH1 profile %d\n",profile);
	ret = mxuvc_video_set_profile(CH1, PROFILE_BASELINE);
	if(ret < 0)
		goto error;
	printf("changed profile to PROFILE_BASELINE\n");
	ret = mxuvc_video_get_profile(CH1, &profile);
	if(ret < 0)
		goto error;
	printf("CH1 profile %d\n",profile);
	
	/***** Start audio  streaming ************/
	ret = mxuvc_audio_start(AUD_CH2);
	if(ret < 0)
		goto error;
	/***** Start video streaming ************/
	for(channel=CH1; channel<ch_count ; channel++){
		/* Start streaming */
		ret = mxuvc_video_start(channel);
		if(ret < 0)
			goto error;
	}

	/***** alert ************/
	if (audio_threshold > 0){
		//init audio alarm 
		ret = mxuvc_alert_init();
		if(ret < 0)
			goto error;
                ret = mxuvc_alert_audio_register_cb(&aalarm_callback, (void *)&adata);
                if (ret){
                        printf("mxuvc_alert_audio_register_callback() failed\n");
                        goto error;
                }


		ret = mxuvc_alert_audio_enable();

		if (ret){
			printf("mxuvc_alert_audio_enable() failed\n");
			goto error;
		}

		//enable audio alert again to test
		ret = mxuvc_alert_audio_set_threshold(50);

		if (ret){
		    printf("mxuvc_alert_audio_set_threshold() failed\n");
		    goto error;
		}
	}
	
	//sleep
	usleep(50000);

	/* Main 'loop' */
	if (argc > 1){
		counter = atoi(argv[1]);
	} else
		counter = 15;

	while(counter--) {
		if (!mxuvc_audio_alive() || !mxuvc_video_alive())
			goto error;
		printf("\r%i secs left", counter+1);

		//video_testing(counter);
		//get current audio intensity
		ret = mxuvc_get_current_audio_intensity(&current_audio_intensity);
		if(ret){
			printf("mxuvc_get_current_audio_intensity() failed\n");
			goto error;
		}else
			printf("Current audio intensity %ddB\n",current_audio_intensity);

		if(counter == 10){
			mxuvc_alert_audio_disable();
		}

		if(counter == 8){
			ret = mxuvc_alert_audio_set_threshold(audio_threshold);
			if (ret){
			    printf("mxuvc_alert_audio_set_threshold() failed\n");
			    goto error;
			}
			ret = mxuvc_alert_audio_enable();

			if (ret){
			    printf("mxuvc_alert_audio_enable() failed\n");
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
	mxuvc_audio_deinit();

	close_fds();	

	printf("Failed\n");
	return 1;
}
