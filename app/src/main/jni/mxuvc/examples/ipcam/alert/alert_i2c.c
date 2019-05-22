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

#include "mxuvc.h"

//uac related globals
FILE *fd_aud = NULL;
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
			printf("sam_interval %lld upper_limit %lld low_limit %lld\n",
					sam_interval,upper_sam_interval,lower_sam_interval);
		}

		if((((ts-prev_ts) > upper_sam_interval) || ((ts-prev_ts) < lower_sam_interval)) && (prev_ts))
			printf("out of range: %lld, last ts %lld preset ts %lld\n",(ts-prev_ts),prev_ts, ts);

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
		free(fname);
	}


	fwrite(buffer, size, 1, fd[ch][fmt]);
	mxuvc_video_cb_buf_done(ch, info.buf_index);
}

void i2c_notify_cb(int value)
{
	printf("\ni2c_notify_cb 0x%x\n",value);
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
	if(fd_aud != NULL)
		fclose(fd_aud);

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
	int ch_count = 0;
	long channel;
	char data;
	int i2c_instance = 1; 
	int i2c_device_type = 3; 
	//at present this test is for AR0330 sendor to read the exposure value
	//while covering the lense and uncovering it i2c alert will come.
	uint16_t i2c_device_addr = 0x20;
	uint16_t chip_id_reg = 0x3000;
	uint16_t chip_id_reg_value = 0x26;
	int handle;
	uint32_t read_len = 0;
	uint16_t width, hight;
	int framerate = 0;
	int goplen = 0;
	int audio_threshold = 0;
	int current_audio_intensity = 0;
	uint16_t regval = 0;

	fd_aud = NULL;
	
	//initialize audio part **************************
	ret = mxuvc_audio_init("alsa","device = Condor");
	if(ret<0)
		goto error;

	ret = mxuvc_alert_init();
	if(ret < 0)
		goto error;
	
	ret = mxuvc_audio_register_cb(AUD_CH2,(mxuvc_audio_cb_t) audio_cb, NULL);
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
	printf("Total Channel count: %ld\n",ch_count);
	//remove raw channel from count
	ch_count = ch_count - 1;

	for(channel=CH2 ; channel<ch_count; channel++)
	{
		ret = mxuvc_video_register_cb(channel, ch_cb, (void*)channel);
		if(ret < 0)
			goto error;
	}

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
	
	if (audio_threshold > 0){
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

	//start i2c notification
	handle = mxuvc_i2c_open(i2c_instance, i2c_device_type, i2c_device_addr, 
			chip_id_reg, chip_id_reg_value);
	if(handle <= 0){
		printf("err: mxuvc_i2c_open failed\n");
		goto error;	
	}
#if 0			
	ret = mxuvc_i2c_notification_read_register(handle, 0x10, &regval, 1);
	if(ret < 0){
		printf("err: mxuvc_i2c_notification_read_register failed\n");
		goto error;	
	}
	printf("regval %x\n",regval);

	ret = mxuvc_i2c_notification_write_register(handle, 0x10, 0x2, 1);
	if(ret < 0){
		printf("err: mxuvc_i2c_notification_write_register failed\n");
		goto error;	
	}
	ret = mxuvc_i2c_notification_read_register(handle, 0x10, &regval, 1);
	if(ret < 0){
		printf("err: mxuvc_i2c_notification_read_register failed\n");
		goto error;	
	}
	printf("regval %x\n",regval);
#endif
	//sleep(1);
	read_len = 1; //max 2 bytes can be read together
	ret = mxuvc_i2c_register_notification_callback(
					handle,
					i2c_notify_cb,
					500, //polling time
					0x3012, //exposure value upper byte, while lens is covered it will be around 0x7 & in open condition it will be 0.
					read_len);
	if(ret < 0){
		printf("err: mxuvc_i2c_register_notification_callback failed\n");
		goto error;	
	}
	ret = mxuvc_i2c_notification_add_threshold(
					handle,
					0x1,
					0x2);
	if(ret < 0){
		printf("err: mxuvc_i2c_notification_add_threshold failed\n");
		goto error;	
	}
	ret = mxuvc_i2c_notification_add_threshold(
					handle,
					0x4,
					0x6);
	if(ret < 0){
		printf("err: mxuvc_i2c_notification_add_threshold failed\n");
		goto error;	
	}
	/* Main 'loop' */
	if (argc > 1){
		counter = atoi(argv[1]);
	} else
		counter = 15;

	while(counter--) {
		if (!mxuvc_audio_alive() || !mxuvc_video_alive())
			goto error;

		printf("\r%i secs left", counter);

		if(counter==2){
			ret = mxuvc_i2c_notification_remove_threshold(
				handle,
				0x1,
				0x2);
			if(ret < 0){
				printf("err: mxuvc_i2c_notification_remove_threshold failed\n");
				goto error;	
			}	
            ret = mxuvc_i2c_notification_remove_threshold(
				handle,
				0x4,
				0x6);
            if(ret < 0){
				printf("err: mxuvc_i2c_notification_remove_threshold failed\n");
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
	ret = mxuvc_i2c_close(handle);	

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
	mxuvc_alert_deinit();
	mxuvc_audio_deinit();
	mxuvc_video_deinit();
	mxuvc_custom_control_deinit();

	close_fds();	

	printf("Failed\n");
	return 1;
}
