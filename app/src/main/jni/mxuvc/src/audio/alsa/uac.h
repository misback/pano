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

#ifndef _AUDIO_H_
#define _AUDIO_H_

/* Audio interface  is less bandwidth (cpu share) consuming interface however it is time sensitive interface.
 * If the AudioEngine thread is not scheduled within the sample duration time(audio_duration_ms),
 * then Applications may start experiencing the Audio Lag or other Audio quality related issues.
 * Applications need to fine tune below parameters to avoid  starving of AudioEngine thread.
 * Note: Use below parametrs, only if you think AudioEngine thread is getting starved
 * due to other high cpu intensive threads/processes are in the system.
 *
 *	MXUVC_AUDIO_ALSA_ENABLE_PTHREAD_PRIORITY to Enable the Real time scheduling.
 *	MXUVC_AUDIO_ALSA_SET_PTHREAD_SCHED_POLICY to set the Linux Scheduling Policy
 *	MXUVC_AUDIO_ALSA_SET_PTHREAD_SCHED_PRIORITY to set the thread Priority.
 */

#define MXUVC_AUDIO_ALSA_ENABLE_PTHREAD_PRIORITY 0  /* Default it is set to Zero */
#define MXUVC_AUDIO_ALSA_SET_PTHREAD_SCHED_POLICY SCHED_RR
#define MXUVC_AUDIO_ALSA_SET_PTHREAD_SCHED_PRIORITY 10

/*
 *
 * Type definitions
 *
 */

/* Instructs the function to use periods */
typedef enum
{
	DEVICE_ALSA_AUDIO_DONT_USE_PERIODS = 0,
	DEVICE_ALSA_AUDIO_USE_PERIODS
} t_device_alsa_audio_use_periods_e;


/*
 *
 * Constant defines
 *
 */
#if (SND_LIB_VERSION > 0x10000)
#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#endif


/* 
 * AUDIO_PERIOD_DEFAULT can be tuned based actual behavior on the platform.
 *
 * Note: More the Period, 
 *	 more the lag in audio since alsa maintains so many buffers of each audio_duration_ms.
 */
#define AUDIO_PERIOD_DEFAULT		3	/* Set the alsa_period default to 4 */

/*
 * Set the Maximum alsa read retry count, 
 * Stop the AudioEngine if alsa_read failes 
 * continuously for ALSA_READ_MAX_RETRY_COUNT times.
 */
#define ALSA_READ_MAX_RETRY_COUNT   5

/* Set whether to use periods or not*/
#define DEFAULT_DEVICE_ALSA_AUDIO_PERIODS  (DEVICE_ALSA_AUDIO_USE_PERIODS)

#define SAMPLE_RATE_48KHZ		48000
#define SAMPLE_RATE_441KHZ		44100
#define SAMPLE_RATE_32KHZ		32000
#define SAMPLE_RATE_24KHZ		24000
#define SAMPLE_RATE_16KHZ		16000
#define SAMPLE_RATE_8KHZ		8000

/* mono */
#define CHANNEL_COUNT_MONO		1
/* stero */
#define CHANNEL_COUNT_STEREO		2

/*
 *
 * Static functions declarations.
 *
 */

struct audio_stream;
static int alsa_start (snd_pcm_t * p_alsa_handle);
static int find_one_ms_data_len (int *one_ms_data_len, snd_pcm_format_t alsa_format, unsigned int channel_count);
static int alsa_mixer_init(char* device_name);
static void * AudioEngine (void *ptr);
static void free_resources(void);

#endif //_AUDIO_H_
