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
#include <getopt.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <pthread.h>
#include <inttypes.h>

#include "mxuvc.h"
FILE *fd[NUM_AUDIO_CHANNELS];
#define OBJECTTYPE_AACLC (2)
#define ADTS_HEADER_LEN (7)

static const int SampleFreq[12] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000
};

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

// default sampling frequency 
int samp_freq = 16000;
#define BOARD_SAMPLE_RATE 16000
uint64_t sam_interval; 

uint64_t prev_ts = 0;

uint64_t permissible_range = 10; //in percentage

uint64_t upper_sam_interval;
uint64_t lower_sam_interval;

static void audio_cb(unsigned char *buffer, unsigned int size,
		audio_format_t format, uint64_t ts, void *user_data, audio_params_t *param)
{
	unsigned char adtsHeader[7];
	audio_channel_t ch = (audio_channel_t) user_data;

	switch(format) {
	case AUD_FORMAT_AAC_RAW: //TBD
		if(fd[ch] == NULL){
			fd[ch] = fopen("out.audio.aac", "w");
			//calculate required sampling interval
			//(1024/sam_freq)*90  //90 is resampler freq
			sam_interval = (uint64_t)(((float)(1024*1000/samp_freq))*90);
			uint64_t percent = (uint64_t)(sam_interval*permissible_range)/100;
			upper_sam_interval = (uint64_t)(sam_interval + percent);
			lower_sam_interval = (uint64_t)(sam_interval - percent);
			printf("sam_interval %" PRIu64 "upper_limit %" PRIu64 " low_limit %" PRIu64 "\n",
					sam_interval,upper_sam_interval,lower_sam_interval);
		}

		if((((ts-prev_ts) > upper_sam_interval) || ((ts-prev_ts) < lower_sam_interval)) && (prev_ts))
			printf("out of range: %" PRIu64 ", last ts %" PRIu64 " preset ts %" PRIu64 "\n",(ts-prev_ts),prev_ts, ts);

		prev_ts = ts;
			
		if(param->samplefreq != samp_freq)	
			printf("Wrong sampling freq, expected %d hz received pkt with %dhz\n",samp_freq,param->samplefreq);

		get_adts_header(param, adtsHeader);
		if(fd[ch] != NULL)
			fwrite(adtsHeader, ADTS_HEADER_LEN, 1, fd[ch]);
		break;
	case AUD_FORMAT_PCM_RAW:
		if(fd[ch] == NULL)
			fd[ch] = fopen("out.audio.pcm", "w");
		break;
	case AUD_FORMAT_OPUS_RAW:
		if(fd[ch] == NULL)
			fd[ch] = fopen("out.audio.opus", "w");
		break;
	default:
		printf("Audio format not supported\n");
		return;
	}

	if(fd[ch] != NULL)
		fwrite(buffer, size, 1, fd[ch]);
}

static void ch_cb(unsigned char *buffer, unsigned int size,
		video_info_t info, void *user_data)
{
	//dummy to start video
}

static void close_fds() {
	int i;
	for(i=0;i<NUM_AUDIO_CHANNELS;i++)
	{
		if ( fd[i] != NULL)
			fclose(fd[i]);
	}
}

static void usage(void)
{
	printf(
			"\nUSAGE: capture_audmux [OPTION]\n"
			"\nOPTION:\n"
			"-h, --help              help\n"
			"-e, --aec               Flag to enable/disable AEC (1 to enable, 0 to disable - default 0)\n"
			"-r, --rate              Audio samplrate 8000|16000|24000- default 16000\n"
			"-s, --stream            Audio capture from specified channel - default 'ch2'\n"
			"                        ch1 - audio capture from 1st channel (PCM data)\n"
			"                        ch2 - audio capture from 2nd channel (Encoded data - AAC or OPUS)\n"
			"                        all - audio capture from all channels (both PCM and encoded data)\n"
			"-g, --gain              Mic gain (range: 0 to 100) - default 100\n"
			"-t, --duration          Capture duration in seconds - default 10 seconds\n"
			"\nEXAMPLE:\n"
			"\ncapture_audmux -e 0 -t 10\n"
			"\n");
}

int main(int argc, char **argv)
{
	int ret;
	int counter=10;
	int bitrate;
	audclk_mode_t mode = 0;
	int aud_frm_intrvl = 0;
	audio_channel_t aud_ch = AUD_CH2;
	int mic_gain=100; 
	int aec_enable = 0;
	//asp_metadata current_audio_stats;

	int option_index;
	int c;
	static const char short_options[] = "he:r:s:g:t:";
	static const struct option long_options[] = {
            {"help", 0, 0, 'h'},
            {"aec", 2, 0, 'e'},
            {"rate", 2, 0, 'r'},
            {"stream", 2, 0, 's'},
            {"gain", 2, 0, 'g'},
            {"duration", 2, 0, 't'},
            {0,0,0,0},
	};

	while ((c = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1)
	{
		switch (c)
		{
		case 'h':
			usage();
			return -1;

		case 'e':
			aec_enable = strtol(optarg, NULL, 0);
			break;

		case 'r':
			samp_freq = strtol(optarg, NULL, 0);
			break;

		case 's':
			if (strcasecmp(optarg, "ch1") == 0)
				aud_ch = AUD_CH1;
			else if (strcasecmp(optarg, "ch2") == 0)
				aud_ch = AUD_CH2;
			else if (strcasecmp(optarg, "all") == 0)
				aud_ch = NUM_AUDIO_CHANNELS;
			else
			{
				printf("invalid stream %s supplied\n",optarg);
				return -1;
			}
			break;

		case 'g':
			mic_gain = strtol(optarg, NULL, 0);
			if (mic_gain < 0 || mic_gain > 100)
			{
				mic_gain = 100;
			}
			break;

		case 't':
			counter = strtol(optarg, NULL, 0);
			if (counter <= 0)
			{
				counter = 10;
			}
			break;

		default:
			fprintf(stderr,"Try `%s --help' for more information.\n", argv[0]);
			return -1;
		}
	}

	int i;
	for(i=0;i<NUM_AUDIO_CHANNELS;i++)
		fd[i] = NULL;

	ret = mxuvc_video_init("v4l2","dev_offset=0");
	if(ret < 0)
		return 1;

	/* Register callback functions */
	ret = mxuvc_video_register_cb(CH1, ch_cb, (void*)CH1);
	if(ret < 0)
		goto error;

	ret = mxuvc_audio_init("alsa","device = Condor");
	if(ret<0)
		goto error;

	if(aud_ch == NUM_AUDIO_CHANNELS)
	{
		audio_channel_t ch;
		for(ch=AUD_CH1; ch<NUM_AUDIO_CHANNELS; ch++)
		{
			ret = mxuvc_audio_register_cb(ch, audio_cb, (void*)ch);
			if(ret < 0)
				goto error;
		}
	}
	else
	{
		ret = mxuvc_audio_register_cb(aud_ch, audio_cb, (void*)aud_ch);
		if(ret < 0)
			goto error;
	}

	mxuvc_audio_set_volume(mic_gain);
	if(ret < 0)
		goto error;

	ret = mxuvc_audio_get_bitrate(&bitrate);
	if(ret < 0)
		goto error;
	printf("audio bitrate %d\n",bitrate);

	if(aud_ch == NUM_AUDIO_CHANNELS)
	{
		audio_channel_t ch;
		for(ch=AUD_CH1; ch<NUM_AUDIO_CHANNELS; ch++)
		{
			ret = mxuvc_audio_set_samplerate(ch, samp_freq);
			if(ret < 0)
				goto error;
		}
	}
	else
	{
		ret = mxuvc_audio_set_samplerate(aud_ch, samp_freq);
      		if(ret < 0)
                	goto error;
	}

	mxuvc_custom_control_init();

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

	sleep(1);

	if(aud_ch == NUM_AUDIO_CHANNELS)
	{
		audio_channel_t ch;
		for(ch=AUD_CH1; ch<NUM_AUDIO_CHANNELS; ch++)
		{
			ret = mxuvc_audio_start(ch);
			if(ret < 0)
				goto error;
		}
	}
	else
	{
		ret = mxuvc_audio_start(aud_ch);
		if(ret < 0)
			goto error;
	}

	while(counter--) {
		if (!mxuvc_audio_alive())
			goto error;
		printf("\r%i secs left", counter+1);
		fflush(stdout);
		sleep(1);

		//Uncomment the code the get the audio stats from firmware
		//ret = mxuvc_custom_control_get_audio_stats(&current_audio_stats);
		//printf("value %d fast %d slow %d\n",current_audio_stats.gain, current_audio_stats.detector.slow_tracker, current_audio_stats.detector.fast_tracker);

		/* uncomment to test Mute/Unmute */
#if 0
		if (counter >= 10)
			mxuvc_audio_set_mic_mute(1);
		else
			mxuvc_audio_set_mic_mute(0);

		if (counter >= 8)
			mxuvc_audio_set_left_mic_mute(1);
		else
			mxuvc_audio_set_left_mic_mute(0);

		if (counter >= 5)
			mxuvc_audio_set_right_mic_mute(1);
		else
			mxuvc_audio_set_right_mic_mute(0);
#endif
	}
	
	/* Stop audio streaming */
	if(aud_ch == NUM_AUDIO_CHANNELS)
	{
		audio_channel_t ch;
		for(ch=AUD_CH1; ch<NUM_AUDIO_CHANNELS; ch++)
		{
			ret = mxuvc_audio_stop(ch);
			if(ret < 0)
				goto error;
		}
	}
	else
	{
		ret = mxuvc_audio_stop(aud_ch);
		if(ret < 0)
			goto error;
	}

	mxuvc_custom_control_deinit();
	mxuvc_audio_deinit();
	mxuvc_video_deinit();
	close_fds();
	return 0;

error:
	mxuvc_audio_deinit();
	close_fds();	

	printf("Failed\n");
	return 1;
}
