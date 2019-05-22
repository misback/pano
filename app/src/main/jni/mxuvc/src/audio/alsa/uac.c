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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <alsa/asoundlib.h>
#include <alsa/control.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "common.h"
#include "mxuvc.h"
#include "uac.h"
#include "qbox.h"

/* Invoke the user callback every AUDIO_DURATION_MS ms worth of data*/
#define AUDIO_DURATION_MS_DEFAULT       10
#define AUDIO_SAMPLING_RATE_DEFAULT     16000

#define MAX_BUFFER_SIZE   (1024*2)
#define NUM_SAMPLES_FOR_AAC   1024		/* AAC works on 1024 samples */

#define START_STATE_OFF      (0)
#define START_STATE_STARTED  (1)

/* default device "values" variable output for individual mic mute functionality 
 * The #define is the value output as part of amixer command to check
 * the details of Condor card. numid=1 is individual mic
 * Command "amixer contents" on condor
 */
#define MIC_2X_CAPTURE_SWITCH 2


/* default device "values" variable output for global mic mute functionality 
 * The #define is the value output as part of amixer command to check
 * the details of Condor card master. numid=2 is master
 * Command "amixer contents" on condor
 */
#define MASTER_MIC_CAPTURE_SWITCH 1

typedef enum{
	AUDIO_PCM_DEVICE = 0,
	AUDIO_MUX_DEVICE,
	MAX_NUM_ALSA_DEVICE
}audio_device_type_t;

/* handles to ALSA device */
typedef struct {
	char dev_name[32];
	audio_device_type_t device_type;
	snd_pcm_t *alsa_handle;
	int current_audio_framesize;
	snd_pcm_format_t current_alsa_format;
	unsigned int current_audio_duration_ms;
	unsigned int current_audio_blocksize_bytes;
	snd_pcm_uframes_t current_audio_period_size;
	unsigned int channel_count;
	volatile int exit_AudioEngine;
	volatile int AudioEngine_started;
	pthread_t thread_id;
	int stop_event_fd[2];
	int status_event_fd[2];
}audio_device_info_t;

static audio_device_info_t audio_device[MAX_NUM_ALSA_DEVICE];

static struct audio_stream {
	audio_channel_t ch;
	audio_device_info_t* adevice;
	mxuvc_audio_cb_t apps_audio_callback_func;
	void *apps_audio_callback_func_user_data;
	audio_format_t format;
	volatile int started;
} audio_stream[NUM_AUDIO_CHANNELS];

/* sound card number */
static int card_number = -1;

/* alsa mixer params */
static unsigned int current_audio_sampling_rate;
static snd_mixer_t* alsa_mixer_handle = NULL;
static snd_mixer_elem_t* mixer_elem = NULL;
static long alsa_mixer_max_volume;
static long alsa_mixer_min_volume;

/* parameters to store the left and right mic state */
static int alsa_ctl_left_mute = 0;
static int alsa_ctl_right_mute = 0;

/* ALSA device handler for capture device.*/
/* TODO try to make this local */
char *audio_device_name = NULL;

/* TODO why is this required? */
static unsigned short max_volume = 100;

const char* mixer_mic_table[] = {
    "Capture",
    "Mic"
};

static int audio_channel_init(audio_channel_t ch);
static void audio_channel_deinit(audio_channel_t ch);

static void* read_pcm_data_thread (void *ptr);
static void* read_encoded_data_thread (void *ptr);
static int get_alsa_card_info (char * dev_name, int *cardnum, int *devcount);
static int get_alsa_hctl_numid(int cardno, int values);
int set_alsa_hctl_func(int numid, char *cmd, int cardnum);
static int alsa_configure (audio_device_info_t* adevice);
static int alsa_read_ready (audio_device_info_t* adevice, unsigned int *pbytes_to_read);
static int alsa_read (audio_device_info_t* adevice, void *p_data_buffer, int data_length);
static int _mxuvc_audio_start(audio_channel_t ch);
static int _mxuvc_audio_stop (audio_channel_t ch);

int audio_channel_init(audio_channel_t ch)
{
    struct audio_stream *astream = &audio_stream[ch];
    astream->ch = ch;

	//Channel AUD_CH1 delivers PCM samples and AUD_CH2 delivers encoded data (such as AAC or OPUS).
	//So configure alsa according to audio channel selected.
    if (ch == AUD_CH1) {
    	astream->adevice = &audio_device[AUDIO_PCM_DEVICE];
	    astream->adevice->device_type = AUDIO_PCM_DEVICE;
    } else {
    	astream->adevice = &audio_device[AUDIO_MUX_DEVICE];
    	astream->adevice->device_type = AUDIO_MUX_DEVICE;
    }
    astream->started = START_STATE_OFF;

    return 0;
}

void audio_channel_deinit(audio_channel_t ch)
{
    return;
}

/*
 *
 * mxuvc Audio APIs
 *
 */

/**
 ****************************************************************************
 * @brief Open/Init the Maxim camera's Mic interface using ALSA APIs
 *
 * This function will open the Maxim camera Mic interface,
 * initialise the device and allocate resources ready for processing camera's
 * Mic data.
 *
 * @param[in]  backend - This string represents the backend interface used.
 *  alsa => to use ALSA framework.
 *  libusb-uac => to use libusb framework
 * Note : the selection of alsa/libusb is compile time decision.
 *
 * @param[in]  options - This string consists of below paraters to be set
 *      device - The name of Maxim Camera.
 *      audio_sampling_rate - The sampling rate to open the device
 *          data  will be read from audio device
 * Below is the format to be used to create the options string.
 *      "device = MAX64380;audio_sampling_rate = 16000"
 *
 * @return 0 when the function completes successfully, or -1 to indicates an error
 *****************************************************************************/
int mxuvc_audio_init (const char *backend, const char *options)
{
    RECORD("\"%s\", \"%s\"", backend, options);
    int ret = 0;
    char *str = NULL, *tmp_str = NULL, *opt, *value;
    char dev_name[20];
    int device_count, channel, dev_id;

    TRACE("Initializing the audio\n");
    /* Check that the correct video backend was requested */
    if (strncmp (backend, "alsa", 4))
    {
        ERROR (-1, "The audio_ backend requested (%s) does not match "
               "the implemented one (libusb-uac)", backend);
    }

    /* Set init parameters to their default values */
    current_audio_sampling_rate = AUDIO_SAMPLING_RATE_DEFAULT;

    /* Copy the options string to a new buffer since next_opt() needs
     * non const strings and options could be a const string
     * */
    if (options != NULL)
    {
        str = (char *) malloc (strlen (options) + 1);
        /* hold the address in tmp; needed later to free it */
        tmp_str = str;
        strncpy (str, options, strlen (options));
        *(str + strlen (options)) = '\0';
    }

    /* Get backend option from the option string */
    ret = next_opt (str, &opt, &value);
    while (ret == 0)
    {
        if (strncmp (opt, "device", 6) == 0)
        {
            audio_device_name = malloc(strlen(value) + 1);
            strncpy(audio_device_name, value, strlen(value));
            audio_device_name[strlen(value)] = '\0';
        }
        else if (strncmp (opt, "audio_sampling_rate", 19) == 0)
        {
            current_audio_sampling_rate =
                (unsigned int) strtoul (value, NULL, 10);
        }
        ret = next_opt (NULL, &opt, &value);
    }

    /* Parsing done free the str */
    free (tmp_str);

    /* find sound card information */
    ret = get_alsa_card_info(audio_device_name, &card_number, &device_count);

    if(ret > 0) {
        ERROR_NORET("Unable to find soundcard '%s'", audio_device_name);
        if(audio_device_name) {
            free(audio_device_name);
            audio_device_name = NULL;
        }
        return -1;
    }
    if(card_number < 0 || device_count <= 0) {
        ERROR_NORET("Unable to find soundcard '%s'", audio_device_name);
        if(audio_device_name) {
            free(audio_device_name);
            audio_device_name = NULL;
        }
        return -1;
    }

    TRACE("Detected %s: card %d with %d device/s\n",audio_device_name, card_number,device_count);

    /* Display the values we are going to use */
    TRACE("Using device = %s\n", audio_device_name);
    TRACE("Using audio_sampling_rate = %i\n",  current_audio_sampling_rate);

    /* get number of devices in the card and initialize all */
    for (dev_id=0 ; dev_id<device_count ; dev_id++) {
        sprintf(dev_name, "hw:%d,%d", card_number, dev_id);
        TRACE("device %d: %s\n",dev_id, dev_name);

        strcpy(audio_device[dev_id].dev_name, dev_name);

        audio_device[dev_id].exit_AudioEngine = 0;
        audio_device[dev_id].thread_id = -1;

        /* Create event file descriptors */
        pipe(audio_device[dev_id].status_event_fd);
        pipe(audio_device[dev_id].stop_event_fd);

        /* Initial counter is 1 so that the first mxuvc_audio_start() does not
           wait for a previous stop to complete */
        int u = -1;
        write(audio_device[dev_id].stop_event_fd[1], &u, sizeof(int));
    }

    /* initialize audio channels */
    for (channel=AUD_CH1 ; channel<NUM_AUDIO_CHANNELS ; channel++) {
        ret = audio_channel_init(channel);
        if(ret < 0) {
            if(audio_device_name) {
                free(audio_device_name);
                audio_device_name = NULL;
            }
            ERROR(-1, "Unable to initialize audio channel");
        }
    }

    /* Init alsa mixer */
    if ( alsa_mixer_init(audio_device_name) < 0)
    {
        free_resources();
        ERROR( -1, "[%d] ERROR alsa_mixer_init failed", __LINE__);
    }

    /* open half duplex write */
    TRACE2("[%d] Sample rate = %i\n", __LINE__, current_audio_sampling_rate);

    return 0;
}

/**
 ****************************************************************************
 * @brief Register the Application's Callback function with the Mxuvc.
 *
 * The Maxim camera must have been opened by calling mxuvc_audio_init()
 *
 * @param[in]   func -  Application's Callback function address.
 * @param[out]  user_data - Pointer to Application Provite data.
 * @return 0.
 *****************************************************************************/
int mxuvc_audio_register_cb (audio_channel_t ch, mxuvc_audio_cb_t func, void *user_data)
{
    struct audio_stream *astream = &audio_stream[ch];

    RECORD("%p, %p",func, user_data);
    astream->apps_audio_callback_func = func;
    astream->apps_audio_callback_func_user_data = user_data;
    return 0;
}


/**
 ****************************************************************************
 * @brief This function starts the Audio capture.
 *
 * This function creates a thread and
 * @param[in] - None
 * @return 0 when the function completes successfully, or -1 to indicates an error
 *****************************************************************************/
int _mxuvc_audio_start(audio_channel_t ch)
{
    ssize_t size;;
    int u;

    TRACE("Starting audio on channel %d\n", ch);

    struct audio_stream *astream = &audio_stream[ch];

    if (astream->started == START_STATE_STARTED)
        ERROR(-1, "Channel %d already started", ch);

    /* Wait for any previous stop to be completed.
       We wait here instead of inside mxuvc_audio_stop() otherwise
       mxuvc_audio_stop() would deadlock when called from within
       the audio callback.
     */

    size = read(astream->adevice->stop_event_fd[0], &u, sizeof(int));
    if (size != sizeof(int))
        ERROR(-1, "Communication with the audio thread failed");

    if(astream->adevice->AudioEngine_started)
    {
    	astream->started = START_STATE_STARTED;
    	return 0;
    }

    /* Start the audio thread */
    pthread_attr_t thread_att;
    pthread_attr_init(&thread_att);
    pthread_attr_setdetachstate(&thread_att, PTHREAD_CREATE_DETACHED);

    if(astream->adevice->device_type == AUDIO_PCM_DEVICE)
    {
        int ret = pthread_create (&astream->adevice->thread_id, &thread_att, &read_pcm_data_thread, astream->adevice);
        if (ret != 0)
        {
            astream->started = START_STATE_OFF;
            ERROR(-1," Unable to start the audio. Failed to create the "
                  "pthread: err = %d", ret);
        }
    }
    else
    {
        int ret = pthread_create (&astream->adevice->thread_id, &thread_att, &read_encoded_data_thread, astream->adevice);
        if (ret != 0)
        {
            astream->started = START_STATE_OFF;
            ERROR(-1," Unable to start the audio. Failed to create the "
                  "pthread: err = %d", ret);
        }
    }
    pthread_attr_destroy(&thread_att);

    /* Wait for the audio thread to report audio status */
    size = read(astream->adevice->status_event_fd[0], &u, sizeof(int));
    if (size != sizeof(int))
        ERROR(-1, "Communication with the audio thread failed");

    if(astream->adevice->AudioEngine_started)
    	astream->started = START_STATE_STARTED;
    else
        ERROR(-1, "Audio start failed for channel %d",ch);

    return 0;
}

int mxuvc_audio_start (audio_channel_t ch)
{
    RECORD("");
    return _mxuvc_audio_start(ch);
}

/**
 ****************************************************************************
 * @brief Close the Maxim camera
 *
 * This function will close the Maxim camera.  The camera must have first been
 * opened by a call to #mxuvcOpen
 *
 * @param[in] - NONE
 * @return 0
 *****************************************************************************/
int _mxuvc_audio_stop (audio_channel_t ch)
{
    int ret;

    TRACE("Stopping audio\n");
    struct audio_stream *astream = &audio_stream[ch];

    if (astream->adevice->AudioEngine_started == 0
            || astream->started == START_STATE_OFF) {
        printf("Unable to stop the audio: audio is not started.\n");
        ret = -1;
    } else {
    	astream->adevice->exit_AudioEngine = 1;
        ret = 0;
    }
    astream->started = START_STATE_OFF;

    return ret;

}

int mxuvc_audio_stop (audio_channel_t ch)
{
    RECORD("");
    return _mxuvc_audio_stop(ch);
}


/**
 ****************************************************************************
 * @brief Set the sample rate of the Maxim camera.
 *
 * This function will update the param 'sampleRate' with the current sample rate being used
 *
 * @param[in] samplingFr - The current sample rate of the Maxim camera audio stream
 * @return 0 when the function completes successfully, or -1 to indicates an error
 *****************************************************************************/
int mxuvc_audio_set_samplerate (audio_channel_t ch, int samplingFr)
{
    int alsa_err;
    struct audio_stream *astream = &audio_stream[ch];

    RECORD("%i", samplingFr);

    TRACE("Setting audio sample rate to %i.\n", samplingFr);

    current_audio_sampling_rate = samplingFr;

    if (astream->started) {
        alsa_err = _mxuvc_audio_stop(ch);
        CHECK_ERROR(alsa_err < 0, -1,
                    "Could not set the audio sample rate to %i",
                    samplingFr);

        /* configure the audio capture device */
        alsa_err = alsa_configure (astream->adevice);
        CHECK_ERROR (alsa_err < 0, -1,
                     "[%d] alsa_configure failed", __LINE__);

        alsa_err = _mxuvc_audio_start(ch);
        CHECK_ERROR (alsa_err < 0, -1,
                     "[%d] function mxuvc_audio_start failed",
                     __LINE__);
    }

    if ( samplingFr != (int) current_audio_sampling_rate )
    {
        ERROR (-1, "[%d] Not able to set the requested sampling Rate", __LINE__);
    }

    return 0;
}

/* Get the current sample rate */
int mxuvc_audio_get_samplerate(audio_channel_t ch)
{
    RECORD("");
    return current_audio_sampling_rate;
}


/**
 ****************************************************************************
 * @brief This function to chcek the audio capture started and also to check if
 * it is still happening.
 *
 * @param[in]   NONE
 * @return 1 if audio capture already started.
 * @return 0 if audio capture is not started or halted due to some error.
 *****************************************************************************/
int mxuvc_audio_alive ()
{
    RECORD("");

    int i;
    for(i=0;i<MAX_NUM_ALSA_DEVICE;i++)
    {
    	if(audio_device[i].AudioEngine_started && !audio_device[i].exit_AudioEngine)
    		return 1;
    }

    return 0;
}

/**
 ****************************************************************************
 * @brief Close the Maxim camera
 *
 * This function will close the Maxim camera Mic interface and frees all allocated resources.
 * @return 0
 *****************************************************************************/
int mxuvc_audio_deinit ()
{
    RECORD("");
    int channel;

    for (channel=AUD_CH1; channel<NUM_AUDIO_CHANNELS; channel++) {
        struct audio_stream *astream = &audio_stream[channel];
        if (astream->started) {
            _mxuvc_audio_stop(channel);
        }
        while(astream->adevice->AudioEngine_started){
            //wait for running thread to exit
            usleep(10000);
        }
        audio_channel_deinit(channel);
    }
    snd_config_update_free_global();

    free_resources();
    TRACE("The audio has been successfully uninitialized\n");
    return 0;
}

/**
 ****************************************************************************
 * @brief this function gets the current Mic Gain/Volume.
 *
 * @param - None
 * @return volume set (range 0 to 100)
 *****************************************************************************/
int mxuvc_audio_get_volume()
{
    RECORD("");

    int err;
    long alsa_volume, alsa_volume_range, volume;

    TRACE("Getting audio volume\n");

    CHECK_ERROR((alsa_mixer_handle == NULL) || (mixer_elem == NULL) ,-1,
                "Unable to get the audio volume. "
                "Error mixer device not opened");

    /* check the device even has a volume control */
    CHECK_ERROR(snd_mixer_selem_has_capture_volume(mixer_elem) == 0 , -1,
                "Unable to get the audio volume. Card %d name '%s' "
                "has no volume control", card_number,
                snd_mixer_selem_get_name(mixer_elem));

    err = snd_mixer_selem_get_capture_volume(mixer_elem,
            SND_MIXER_SCHN_FRONT_LEFT,
            &alsa_volume);
    CHECK_ERROR(err < 0, -1,
                "Unable to get the audio volume. "
                "Error calling snd_mixer_selem_get_capture_volume: %s",
                snd_strerror(err));

    TRACE2("[%d] Alsa volume = %lu (range %lu to %lu)\n", __LINE__,
           (long)alsa_volume,
           alsa_mixer_min_volume,
           alsa_mixer_max_volume);

    alsa_volume_range = alsa_mixer_max_volume - alsa_mixer_min_volume;
    volume = (alsa_volume * max_volume) / alsa_volume_range;

    return (int) volume;
}

/**
 ****************************************************************************
 * @brief This function used to Mute/Unmute the Mic.
 *
 * @param[in] Mute - This variable will have value 0 or 1.
 *           If  1 , Mic is muted
 *           If  0 , Mic is Unmuted
 * @return 0 when the function completes successfully, or -1 to indicates an error
 *****************************************************************************/
int mxuvc_audio_set_mic_mute(int bMute)  /* mute=1, unmute=0 */
{

    int err = -1;
    char cmd[10];

    if ( bMute )
        strcpy(cmd, "off");
    else
        strcpy(cmd, "on");


    int cardnum, devcount;

    err = get_alsa_card_info (audio_device_name, &cardnum, &devcount);
    if ( err < 0 )
    {
        ERROR(-1," [%d] ERROR unable to find the card\n",__LINE__);
    }

    int numid = get_alsa_hctl_numid(cardnum, MASTER_MIC_CAPTURE_SWITCH);
    if ( numid < 0 )
    {
        ERROR(-1, "[%d] ERROR unable to locate the card\n",__LINE__);
    }


    err = set_alsa_hctl_func(numid, cmd, cardnum);
    if ( err < 0 )
    {
        ERROR(-1, "[%d] ERROR unable to set functionality on card\n",__LINE__);
    }
       
    return 0; //SUCCESS;
}

/**
 ****************************************************************************
 * @brief This function used to Mute/Unmute the left Mic.
 *
 * @param[in] Mute - This variable will have value 0 or 1.
 *           If  1 , Mic is muted
 *           If  0 , Mic is Unmuted
 * @return 0 when the function completes successfully, or -1 to indicates an error
 *****************************************************************************/
int mxuvc_audio_set_left_mic_mute(int bMute)
{


    int err = -1;
    char cmd[10];

    if ( bMute )
        strcpy(cmd, "off,");
    else
        strcpy(cmd, "on,");

    if ( alsa_ctl_right_mute )
        strcat(cmd, "off");
    else
        strcat(cmd, "on");

    int cardnum = -1, devcount = -1;

    err = get_alsa_card_info (audio_device_name, &cardnum, &devcount);
    if ( err < 0 )
    {
        ERROR(-1," [%d] ERROR unable to find the card\n",__LINE__);
    }

    int numid = get_alsa_hctl_numid(cardnum, MIC_2X_CAPTURE_SWITCH);
    if ( numid < 0 )
    {
        ERROR(-1, "[%d] ERROR unable to locate the card\n",__LINE__);
    }

    err = set_alsa_hctl_func(numid, cmd, cardnum);
    if ( err < 0 )
    {
         ERROR(-1, "[%d] ERROR unable to set functionality on card\n",__LINE__);
    }

    alsa_ctl_left_mute = bMute;

    return 0;
}


/**
 ****************************************************************************
 * @brief This function used to Mute/Unmute the right Mic.
 *
 * @param[in] Mute - This variable will have value 0 or 1.
 *           If  1 , Mic is muted
 *           If  0 , Mic is Unmuted
 * @return 0 when the function completes successfully, or -1 to indicates an error
 *****************************************************************************/
int mxuvc_audio_set_right_mic_mute(int bMute)
{

    int err = -1;
    char cmd[10];

    if ( alsa_ctl_left_mute )
        strcpy(cmd, "off,");
    else
        strcpy(cmd, "on,");

    if ( bMute )
        strcat(cmd, "off");
    else
        strcat(cmd, "on");

    int cardnum = -1, devcount = -1;

    err = get_alsa_card_info (audio_device_name, &cardnum, &devcount);
    if ( err < 0 )
    {
        ERROR(-1," [%d] ERROR unable to find the card\n",__LINE__);
    }

    int numid = get_alsa_hctl_numid(cardnum, MIC_2X_CAPTURE_SWITCH);
    if ( numid < 0 )
    {
        ERROR(-1, "[%d] ERROR unable to locate the card\n",__LINE__);
    }

    err = set_alsa_hctl_func(numid, cmd, cardnum);
    if ( err < 0 )
    {
         ERROR(-1, "[%d] ERROR unable to set functionality on card\n",__LINE__);
    }

    alsa_ctl_right_mute = bMute;

    return 0;
}

/**
 ****************************************************************************
 * @brief API to increasse/decrese Volume/Gain of Mic.
 *
 * Sets the gain of the device.  Gain is specificed between 0 and 100.
 * where 0 is - Infinity dB and 100 is max gain (min=0, max=100)
 *
 * @param[in] volume - The volume to be set.
 * @return 0 when the function completes successfully, or -1 to indicates an error
 *****************************************************************************/
int mxuvc_audio_set_volume(int user_volume)
{
    RECORD("%i", user_volume);
    int err;
    int alsa_volume;

    TRACE("Setting audio volume to %i\n", user_volume);

    CHECK_ERROR((alsa_mixer_handle == NULL) || (mixer_elem == NULL) ,-1,
                "Unable to set the audio volume to %i. "
                "Error mixer device not opened", user_volume);

    /* check the device even has a volume control */
    CHECK_ERROR(snd_mixer_selem_has_capture_volume(mixer_elem) == 0 , -1,
                "Unable to set the audio volume to %i. Card %d name '%s' "
                "has no volume control", user_volume, card_number,
                snd_mixer_selem_get_name(mixer_elem));

    alsa_volume = ( ( (alsa_mixer_max_volume - alsa_mixer_min_volume) * user_volume ) / max_volume);
    TRACE2("[%d] Alsa volume = %lu (range %lu to %lu)\n", __LINE__,
           (long)alsa_volume,
           alsa_mixer_min_volume,
           alsa_mixer_max_volume);

    /* set the volume for all channels */
    err = snd_mixer_selem_set_capture_volume_all(mixer_elem, (long)alsa_volume);
    CHECK_ERROR(err < 0, -1,
                "Unable to set the audio volume to %i. "
                "Error calling snd_mixer_selem_set_capture_volume_all: %s",
                user_volume, snd_strerror(err));

    return 0;
}

/* static functions which are not exposed to Application */

/*
 * This function is called first before starting the capture.
 * In this function we trigger alsa to start capture of Mic data.
 */
int alsa_start (snd_pcm_t * p_alsa_handle)
{
    snd_pcm_state_t state;
    int err;

    /* Check that the passed handle is legal */
    CHECK_ERROR((p_alsa_handle == (snd_pcm_t *) - 1) || (p_alsa_handle == NULL), -1,
                "[%d] Invalid handle %p", __LINE__, p_alsa_handle);

    state = snd_pcm_state (p_alsa_handle);
    TRACE2("[%d] state:%d\n", __LINE__,state);
    switch (state)
    {
    case SND_PCM_STATE_OPEN:
        break;
    case SND_PCM_STATE_SETUP:
        break;
    case SND_PCM_STATE_DRAINING:
        break;
    case SND_PCM_STATE_PAUSED:
        break;
    case SND_PCM_STATE_SUSPENDED:
        break;
#if (SND_LIB_VERSION > 0x10000)
    case SND_PCM_STATE_DISCONNECTED:
        break;
#endif
    case SND_PCM_STATE_XRUN:
        do
        {
            TRACE2("[%d] Trying snd_pcm_prepare()...", __LINE__);
            err = snd_pcm_prepare (p_alsa_handle);
        }
        while (err < 0);
        err = snd_pcm_start (p_alsa_handle);
        CHECK_ERROR(err < 0, -1,
                    "[%d] Cannot start PCM (%s)", __LINE__,
                    snd_strerror (err));
        break;
    case SND_PCM_STATE_PREPARED:
        err = snd_pcm_start (p_alsa_handle);
        CHECK_ERROR(err < 0, -1,
                    "[%d] Cannot start PCM (%s)", __LINE__,
                    snd_strerror (err));
        break;
    case SND_PCM_STATE_RUNNING:
        break;
    default:
        break;
    }

    return 0;
}

/* might require in future so kept commented */
/*int proces_audio_qbox_frames(struct audio_stream *astream,
                unsigned char *buffer,
                uint32_t buf_length)
{
    uint32_t box_size;
    audio_params_t aparam;
    int ts = 0;
    unsigned char *buf = buffer;
    uint32_t length = buf_length;
    int offset = 0;

    //check if its qbox ?
    while(1){
        if(length < 28){
            TRACE("ERR: wrong qbox packet\n");
            break;
        }

        box_size = get_qbox_frame_size(buf+offset);
        if(box_size == 0){
            TRACE("ERR: wrong qbox packet %d\n",length);
            break;
        }else{
            if(box_size > length)
                break;

            length = length - box_size;
        }

        if(audio_param_parser(&aparam, (buf+offset), box_size) == 0){
            astream->apps_audio_callback_func (aparam.dataptr,(unsigned)aparam.framesize,
                    (int) astream->current_audio_format, ts,
                    astream->apps_audio_callback_func_user_data,
                    &aparam);
        }else{
            TRACE("ERR: audio_param_parser failed\n");
            return 1;
        }
        offset = offset + box_size;
    }

    return length;
}
*/
/*
 * This is executed in separate thread.
 * This function is executed in while loop till the audio_stop is called by the
 * App.
 * The sole functionality of this function is to wait for data, then capture the data
 * from Mic and then call the App registered callback function.
 */
void* read_pcm_data_thread (void *ptr)
{
    struct timeval tv;
    uint64_t ts64 = 0, tsec = 0, tusec = 0;
    static uint64_t ts = 0;
    unsigned int bytes_to_read = 0;
    int read_len = 0;
    int err = 0;
    unsigned char loop_buff[MAX_BUFFER_SIZE];
    int read_fail_count = 0;
    audio_params_t aparam;
    audio_device_info_t* adevice = (audio_device_info_t*)ptr;

    adevice->AudioEngine_started = 1;

#if MXUVC_AUDIO_ALSA_ENABLE_PTHREAD_PRIORITY
    struct sched_param param;
    int policy = 0;

    policy = MXUVC_AUDIO_ALSA_SET_PTHREAD_SCHED_POLICY;
    param.sched_priority = MXUVC_AUDIO_ALSA_SET_PTHREAD_SCHED_PRIORITY;

    err = pthread_setschedparam(adevice->thread_id, policy, &param);
    if ( err != 0)
    {
        WARNING("[%d] pthread_setschedparam failed  err_code=%d",
                __LINE__, err);
    }

    err = pthread_getschedparam(adevice->thread_id, &policy, &param);
    if ( err != 0)
    {
        WARNING("[%d] pthread_getschedparam failed  err_code=%d",
                __LINE__, err);
    }

    TRACE2("[%d] AudioEngine thread set  priority=%d  policy=%d \n",
           __LINE__,param.sched_priority,policy);

#endif

    ssize_t size;
    int u = 0;

    /* open the audio device */
    TRACE2("[%d] Opening capture device '%s'\n", __LINE__,
    		adevice->dev_name);
    int alsa_err = snd_pcm_open(&(adevice->alsa_handle),
    		(const char *)adevice->dev_name,
                                SND_PCM_STREAM_CAPTURE, 0);
    if (alsa_err < 0) {
        ERROR_NORET ("Error calling snd_pcm_open(CAPTURE) - ");
        err = 1;
        goto label_exitAudioEngine;
    }

    alsa_err = alsa_configure (adevice);
    if (alsa_err < 0) {
        free_resources();
        ERROR_NORET ("[%d] ERROR alsa_configure(read)", __LINE__);
        err = 1;
        snd_pcm_close(adevice->alsa_handle);
        goto label_exitAudioEngine;
    }

    alsa_err = alsa_start (adevice->alsa_handle);
    if (alsa_err < 0) {
        free_resources();
        ERROR_NORET ("[%d]  alsa_start() failed", __LINE__);
        err = 1;
        snd_pcm_close(adevice->alsa_handle);
        goto label_exitAudioEngine;
    }

    /* Notify mxuvc_audio_start() of the status */
    size = write(adevice->status_event_fd[1], &u, sizeof(int));
    if (size != sizeof(int))
        ERROR_NORET("Communication with the audio thread failed\n");

    while (!adevice->exit_AudioEngine)
    {
        if(adevice->device_type == AUDIO_PCM_DEVICE)
        {
            TRACE2("[%d] Audio Loop started\n",__LINE__);
            /* Wait for audio data */
            alsa_err = snd_pcm_wait (adevice->alsa_handle, adevice->current_audio_duration_ms * 2);
            if (alsa_err == -ENODEV)
            {
                ERROR_NORET("[%d] Capture device %s removed for reason: %s; Stopping the AudioEngine",
                                    __LINE__, audio_device_name, snd_strerror(alsa_err));
                adevice->exit_AudioEngine = 1;
                break;
            }

            /* check the data in audio input and output */
            alsa_read_ready (adevice, &bytes_to_read);

            TRACE2("bytes to read: %i\n", bytes_to_read);

            /* Read all frames in the input buffer */
            while (bytes_to_read >= adevice->current_audio_blocksize_bytes)
            {
                if (adevice->exit_AudioEngine)
                    break;

                if (bytes_to_read >= adevice->current_audio_blocksize_bytes)
                {
                    if (adevice->exit_AudioEngine)
                        break;
                    /* read one block */
                    read_len =
                        alsa_read (adevice, loop_buff,
                        		adevice->current_audio_blocksize_bytes);
                    if ( read_len > 0 )
                    {
                        /* Reset retry count */
                        if (read_fail_count)
                            read_fail_count = 0;

                        /* Create a timestamp here */
                        gettimeofday (&tv, NULL);
                        tsec = (uint64_t) tv.tv_sec;
                        tusec = (uint64_t) tv.tv_usec;
                        ts64 = ( ( (tsec * 1000000 + tusec) * 9) / 100 );
                        ts = (uint64_t) (ts64 & 0xffffffff);

                        /*Call the App's callback and pass the captured audio data */
                    	struct audio_stream *astream = &audio_stream[AUD_CH1];
                    	if(astream->apps_audio_callback_func && (astream->started == START_STATE_STARTED))
                    	{
                            astream->apps_audio_callback_func (loop_buff, (unsigned) read_len,
                                                           AUD_FORMAT_PCM_RAW, ts,
                                                           astream->apps_audio_callback_func_user_data, &aparam);
                    	}
                    }
                    else if (read_len == -2)
                    {
                        ERROR_NORET("[%d] Capture device %s removed; "
                                    "Stopping the AudioEngine",
                                    __LINE__, audio_device_name);
                        adevice->exit_AudioEngine = 1;
                        break;
                    }
                    else
                    {
                        ++read_fail_count;
                        if ( read_fail_count >= ALSA_READ_MAX_RETRY_COUNT )
                        {
                            ERROR_NORET("[%d] alsa_read() failed continuously "
                                        "%d times; Stopping the AudioEngine",
                                        __LINE__, ALSA_READ_MAX_RETRY_COUNT);
                            adevice->exit_AudioEngine = 1;
                            break;
                        }
                    }
                }
                else
                {
                    WARNING (" --- --- --- too little data to read, "
                             "adding zeros --- --- ---");
                    memset (loop_buff, 0, adevice->current_audio_blocksize_bytes);
                }

                bytes_to_read -= (unsigned ) read_len;
            }
        }
    }

    /* close alsa handle */
    if (snd_pcm_close(adevice->alsa_handle) < 0) {
        ERROR_NORET("Unable to close alsa handle for %s",
        		adevice->dev_name);
    }

    adevice->exit_AudioEngine = 0;

label_exitAudioEngine:
    adevice->AudioEngine_started=0;

    if(err)
    {
        /* added to notify _mxuvc_audio_start() which is blocked on this status */
        size = write(adevice->status_event_fd[1], &u, sizeof(int));
        if (size != sizeof(int))
            ERROR_NORET("Communication with the audio thread failed\n");
    }

    size = write(adevice->stop_event_fd[1], &u, sizeof(int));
    if (size != sizeof(int))
        ERROR_NORET("Communication with the audio thread failed");

    TRACE2("[%d] Audio Loop Exit\n",__LINE__);

    pthread_detach(pthread_self());
    return NULL;
}

void* read_encoded_data_thread (void *ptr)
{
    static uint64_t ts = 0;
    int read_len = 0;
    int err = 0;
    unsigned char loop_buff[MAX_BUFFER_SIZE];
    int hdr_length = 0;
    int qbox_frame_size = 0;
    audio_params_t aparam;
    int channel_id;
    audio_device_info_t* adevice = (audio_device_info_t*)ptr;

    adevice->AudioEngine_started = 1;

#if MXUVC_AUDIO_ALSA_ENABLE_PTHREAD_PRIORITY
    struct sched_param param;
    int policy = 0;

    policy = MXUVC_AUDIO_ALSA_SET_PTHREAD_SCHED_POLICY;
    param.sched_priority = MXUVC_AUDIO_ALSA_SET_PTHREAD_SCHED_PRIORITY;

    err = pthread_setschedparam(adevice->thread_id, policy, &param);
    if ( err != 0)
    {
        WARNING("[%d] pthread_setschedparam failed  err_code=%d",
                __LINE__, err);
    }

    err = pthread_getschedparam(adevice->thread_id, &policy, &param);
    if ( err != 0)
    {
        WARNING("[%d] pthread_getschedparam failed  err_code=%d",
                __LINE__, err);
    }

    TRACE2("[%d] AudioEngine thread set  priority=%d  policy=%d \n",
           __LINE__,param.sched_priority,policy);

#endif

    ssize_t size;
    int u = 0;
    uint32_t analytics;

    /* open the audio device */
    TRACE2("[%d] Opening capture device '%s'\n", __LINE__,
    		adevice->dev_name);
    int alsa_err = snd_pcm_open(&(adevice->alsa_handle),
    		(const char *)adevice->dev_name,
                                SND_PCM_STREAM_CAPTURE, 0);
    if (alsa_err < 0) {
        ERROR_NORET ("Error calling snd_pcm_open(CAPTURE) - ");
        err = 1;
        goto label_exitAudioEngine;
    }

    alsa_err = alsa_configure (adevice);
    if (alsa_err < 0) {
        free_resources();
        ERROR_NORET ("[%d] ERROR alsa_configure(read)", __LINE__);
        err = 1;
        snd_pcm_close(adevice->alsa_handle);
        goto label_exitAudioEngine;
    }

    alsa_err = alsa_start (adevice->alsa_handle);
    if (alsa_err < 0) {
        free_resources();
        ERROR_NORET ("[%d]  alsa_start() failed", __LINE__);
        err = 1;
        snd_pcm_close(adevice->alsa_handle);
        goto label_exitAudioEngine;
    }

    /* Notify mxuvc_audio_start() of the status */
    size = write(adevice->status_event_fd[1], &u, sizeof(int));
    if (size != sizeof(int))
        ERROR_NORET("Communication with the audio thread failed\n");

    while (!adevice->exit_AudioEngine)
    {
        if(adevice->device_type == AUDIO_MUX_DEVICE)
        {
            alsa_err = snd_pcm_wait (adevice->alsa_handle, adevice->current_audio_duration_ms);
            if (alsa_err == -ENODEV)
            {
               ERROR_NORET("[%d] Capture device %s removed for reason: %s; Stopping the AudioEngine",
                                    __LINE__, audio_device_name, snd_strerror(alsa_err));
                adevice->exit_AudioEngine = 1;
                break;
            }

            /*TODO: discard any audio packets received after stopping audio */

            hdr_length = get_qbox_hdr_size();
            read_len = alsa_read (adevice, loop_buff, hdr_length);
            if (read_len == hdr_length)
            {
                video_format_t fmt;
                uint8_t *data_buf;
                metadata_t metadata;
                qmed_t qmed;

                if (qbox_parse_header(loop_buff, &channel_id,
                                      &fmt, (uint8_t **)&data_buf,
                                      (uint32_t *)&qbox_frame_size, &ts,
                                      &analytics, &metadata, &qmed)) {
                    TRACE("Wrong mux audio format\n");
                    continue;
                }
            }
            else if (read_len == -2)
            {
                 ERROR_NORET("[%d] Capture device %s removed; Stopping the AudioEngine",
                                    __LINE__, audio_device_name);
                 adevice->exit_AudioEngine = 1;
                 break;
            } else
                continue;

            if (qbox_frame_size && ((qbox_frame_size+hdr_length) <= MAX_BUFFER_SIZE))
            {
                read_len = alsa_read(adevice, loop_buff+hdr_length, qbox_frame_size);

                if (qbox_frame_size != read_len)
                    continue;

                if (audio_param_parser(&aparam, loop_buff, qbox_frame_size+hdr_length) == 0)
                {
                    struct audio_stream *astream = &audio_stream[AUD_CH2];
                    /*TODO: Include check for audio format when MUX channel carries more than one audio format */
                    if( astream->apps_audio_callback_func && (astream->started == START_STATE_STARTED))
                    {
                        astream->apps_audio_callback_func (aparam.dataptr,(unsigned)aparam.framesize,
                                                           aparam.format, ts,
                                                           astream->apps_audio_callback_func_user_data,
                                                           &aparam);
                    }
                }
            }else if(qbox_frame_size){
                //read and discard the frame
                unsigned char *dump_buff = NULL;
                dump_buff = (unsigned char *)malloc(qbox_frame_size);
                if(dump_buff == NULL){
                    printf("ERROR: malloc failed\n");
                    break;
                }
                read_len = alsa_read(adevice, dump_buff, qbox_frame_size);
                free(dump_buff);
            }
        }
    }

    /* close alsa handle */
    if (snd_pcm_close(adevice->alsa_handle) < 0) {
        ERROR_NORET("Unable to close alsa handle for %s",
        		adevice->dev_name);
    }

    adevice->exit_AudioEngine = 0;

label_exitAudioEngine:
    adevice->AudioEngine_started=0;

    if(err)
    {
        /* added to notify _mxuvc_audio_start() which is blocked on this status */
        size = write(adevice->status_event_fd[1], &u, sizeof(int));
        if (size != sizeof(int))
            ERROR_NORET("Communication with the audio thread failed\n");
    }

    size = write(adevice->stop_event_fd[1], &u, sizeof(int));
    if (size != sizeof(int))
        ERROR_NORET("Communication with the audio thread failed");

    TRACE2("[%d] Audio Loop Exit\n",__LINE__);
    pthread_detach(pthread_self());
    return NULL;
}

/*
 * This function is handy in calculating the number of bytes need to be read
 * per millisecond calculated based on sample rate, number of channels and
 * format.
 * Note: currently this function only supports format SND_PCM_FORMAT_S16_LE
 */
int find_one_ms_data_len (int *one_ms_data_len, snd_pcm_format_t alsa_format, unsigned int channel_count)
{
    /* Determine a few things about the audio buffers */
    switch (alsa_format)
    {
    case SND_PCM_FORMAT_MU_LAW:
    case SND_PCM_FORMAT_A_LAW:
        TRACE2("[%d] SND_PCM_FORMAT_MU_LAW SND_PCM_FORMAT_A_LAW\n",__LINE__);
        *one_ms_data_len = (current_audio_sampling_rate / 1000) * sizeof (char) * channel_count;
        break;

    case SND_PCM_FORMAT_S16_LE:
        TRACE2("[%d] SND_PCM_FORMAT_S16_LE\n", __LINE__);
        *one_ms_data_len = (current_audio_sampling_rate / 1000) * sizeof (short) * channel_count;
        break;

    case SND_PCM_FORMAT_S8:
        TRACE2("[%d] SND_PCM_FORMAT_S8 %d and %d\n", __LINE__,current_audio_sampling_rate,channel_count);
        *one_ms_data_len = (current_audio_sampling_rate / 1000) * sizeof (char) * channel_count;
        break;

    default:
        ERROR(-1, "[%d] illegal audio format", __LINE__);
        break;
    }

    TRACE2("[%d] *one_ms_data_len = %i\n", __LINE__, *one_ms_data_len);
    return 0;
}

/*
 * This is main main function which does the configuration of opened alsa
 * interface using the current set parameters,
 * Format, sample rate, channel count, sample duration, periods and
 * period_size.
 */
int alsa_configure (audio_device_info_t* adevice)
{

    int alsa_err;
    snd_pcm_hw_params_t *p_hw_params = NULL;
    snd_pcm_uframes_t tmp;
    unsigned int tmp_uint;
    snd_pcm_t *p_alsa_handle = adevice->alsa_handle;
    int one_ms_data_len;

    t_device_alsa_audio_use_periods_e alsa_use_periods =
        DEFAULT_DEVICE_ALSA_AUDIO_PERIODS;

    TRACE2("[%s] Entered, handle %p\n", __FUNCTION__, p_alsa_handle);

    /* Check that the passed handle is legal */
    if ((p_alsa_handle == (snd_pcm_t *) - 1) || (p_alsa_handle == NULL))
    {
        ERROR (-1, "Invalid handle %p", p_alsa_handle);
    }

    /* Allocate space for hardware parameters */
    alsa_err = snd_pcm_hw_params_malloc (&p_hw_params);
    if (alsa_err < 0)
    {
        ERROR( -1, "[%d]  Cannot allocate hardware parameter structure: (%s)", __LINE__, snd_strerror (alsa_err));
    }

    /* This function loads current settings */
    alsa_err = snd_pcm_hw_params_any (p_alsa_handle, p_hw_params);
    if (alsa_err < 0)
    {
        ERROR_NORET ("[%d]  Cannot initialize hardware parameter structure: (%s)", __LINE__, snd_strerror (alsa_err));
        /* Free the hardware params then return error code */
        snd_pcm_hw_params_free (p_hw_params);
        return -1;
    }

    /*
    * ALSA offers choice of interleaved samples (that is left/right for stereo)
    * or 'blocks' of channel data. Interleaved is what we are used to from
    * the OSS way of doing things
    */
    if ((alsa_err = snd_pcm_hw_params_set_access (p_alsa_handle, p_hw_params,
                    SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        ERROR_NORET ("[%d] Cannot set access type (%s)", __LINE__, snd_strerror (alsa_err));
        /* Free the hardware params then return error code */
        snd_pcm_hw_params_free (p_hw_params);
        return -1;
    }

    alsa_err = snd_pcm_hw_params_get_format (p_hw_params, &adevice->current_alsa_format);
    if (alsa_err < 0)
    {
        ERROR_NORET ("[%d] Failed to get format", __LINE__);

        //set default one we are interested
        if(adevice->device_type == AUDIO_PCM_DEVICE) {
    	    adevice->current_alsa_format = SND_PCM_FORMAT_S16_LE;

        }
        else {
    		/* Configuring alsa to single channel and 1 byte per sample
    		 * configuration so that we can capture data in terms of bytes
    		 * from the Raptor2 hardware. AAC/OPUS is VBR encoder which sends
    		 * different no of bytes every time and it will not be possible
    		 * to configure to capture from alsa in terms of fixed no:of samples.
    		 */
        	adevice->current_alsa_format = SND_PCM_FORMAT_S8;
        }
    }

    /*
     * Set audio format .
    */
    alsa_err = snd_pcm_hw_params_set_format (p_alsa_handle, p_hw_params,
    		adevice->current_alsa_format);

    if (alsa_err < 0)
    {
        ERROR_NORET ("[%d] Failed to set audio format: %d", __LINE__,
        		adevice->current_alsa_format);
        /* Free the hardware params then return error code */
        snd_pcm_hw_params_free (p_hw_params);
        return -1;
    }

    /* Set sample rate */
    tmp_uint = current_audio_sampling_rate;
#ifdef ALSA_PCM_NEW_HW_PARAMS_API
    if ((alsa_err = snd_pcm_hw_params_set_rate_near (p_alsa_handle, p_hw_params,
                    &current_audio_sampling_rate,
                    0)) < 0)
    {
        ERROR_NORET ("[%d]  Cannot set sample rate (%s)", __LINE__,
                     snd_strerror (alsa_err));
        snd_pcm_hw_params_free (p_hw_params);
        return -1;
    }
    else
    {
        TRACE2("[%d] Sample rate set to %d Hz\n", __LINE__,
               current_audio_sampling_rate);
    }
#else
    alsa_err = snd_pcm_hw_params_set_rate_near (p_alsa_handle, p_hw_params,
               current_audio_sampling_rate, 0);
    if (alsa_err < 0)
    {
        ERROR_NORET ("[%d] Cannot set sample rate (%s)", __LINE__,
                     snd_strerror (alsa_err));
        snd_pcm_hw_params_free (p_hw_params);
        return -1;
    }
#endif
    if (tmp_uint != current_audio_sampling_rate)
    {

        ERROR_NORET ("[%d] Not able to set requested sample rate =%u instead Sample Rate set to= %u ",
                     __LINE__, tmp_uint, current_audio_sampling_rate);
        snd_pcm_hw_params_free (p_hw_params);
        return -1;
    }

    /* get current supported channel count(mono/stereo) */
    alsa_err = snd_pcm_hw_params_get_channels(p_hw_params, &adevice->channel_count);
    if (alsa_err < 0)
    {
        ERROR_NORET ("[%d]  Cannot get channel count (%s)", __LINE__,
                     snd_strerror (alsa_err));
        /* Free the hardware params then return error code */
        snd_pcm_hw_params_free (p_hw_params);
        return -1;
    }

    alsa_err = snd_pcm_hw_params_set_channels_near (p_alsa_handle, p_hw_params,
                                             &adevice->channel_count);
    if (alsa_err < 0)
    {
        ERROR_NORET ("[%d]  Cannot set channel count (%s)", __LINE__,
                     snd_strerror (alsa_err));
        /* Free the hardware params then return error code */
        snd_pcm_hw_params_free (p_hw_params);
        return -1;
    }
    else
    {
        TRACE2("[%d] Channels set to %u\n", __LINE__,adevice->channel_count);
    }

	/* calculate and set current frame size and duration based on format (PCM/encoded) and channel count */
	if(adevice->device_type == AUDIO_MUX_DEVICE)
	{
        /* in the case of MUX channel, the sample size is configured to 1 byte */
        adevice->current_audio_framesize = (1 * adevice->channel_count);

		/* as AAC works on 1024 audio samples, so calculating frame interval in ms for 1024 samples*/
        adevice->current_audio_duration_ms = (NUM_SAMPLES_FOR_AAC*1000)/current_audio_sampling_rate;
	}
	else
	{
        /* in the case of PCM channel, the sample size is configured to 2 byte */
        adevice->current_audio_framesize = (2 * adevice->channel_count);
        adevice->current_audio_duration_ms = AUDIO_DURATION_MS_DEFAULT;
	}

    /* re-calculate current_audio_blocksize_bytes based on current channel count and samplerate */
    if (find_one_ms_data_len (&one_ms_data_len, adevice->current_alsa_format, adevice->channel_count) != 0)
    {
        ERROR (-1, "[%d] find_one_ms_data_len() failed", __LINE__);
    }

    adevice->current_audio_blocksize_bytes = (one_ms_data_len * adevice->current_audio_duration_ms);

    if (alsa_use_periods == DEVICE_ALSA_AUDIO_USE_PERIODS)
    {
        /*
         * Calculate the period size. Set a period as
         * current_audio_duration_ms of audio data for the given sample rate.
         *  This is in 'frames' - there is no need to take bits per sample or
         *  number of channels into account, ALSA 'knows' how to adjust for
         *  this -:).
         */
        switch (current_audio_sampling_rate)
        {
        case SAMPLE_RATE_48KHZ:
            adevice->current_audio_period_size = (48 * adevice->current_audio_duration_ms);
            break;
        case SAMPLE_RATE_441KHZ:
            adevice->current_audio_period_size =
                ((441 * adevice->current_audio_duration_ms) / 10);
            break;
        case SAMPLE_RATE_32KHZ:
       	    adevice->current_audio_period_size = (32 * adevice->current_audio_duration_ms);
            break;
        case SAMPLE_RATE_24KHZ:
            adevice->current_audio_period_size = (24 * adevice->current_audio_duration_ms);
            break;
        case SAMPLE_RATE_16KHZ:
            adevice->current_audio_period_size = (16 * adevice->current_audio_duration_ms);
            break;
        case SAMPLE_RATE_8KHZ:
            adevice->current_audio_period_size = (8 * adevice->current_audio_duration_ms);
            break;
        default:
            ERROR_NORET ("[%d] sample rate not supported (%u)", __LINE__,
                         current_audio_sampling_rate);
            /* Free the hardware params then return error code */
            snd_pcm_hw_params_free (p_hw_params);
            return -1;
            break;
        }

        /* AAC mode audio duration and period size calculation */
        if(adevice->device_type == AUDIO_MUX_DEVICE) {
        	adevice->current_audio_duration_ms = (MAX_BUFFER_SIZE*1000)/current_audio_sampling_rate;
        	adevice->current_audio_period_size=128; //Tuned for lowest possible sam freq(8khz)
        }

        tmp = adevice->current_audio_period_size;

#ifdef ALSA_PCM_NEW_HW_PARAMS_API
        alsa_err =
            snd_pcm_hw_params_set_period_size_near (p_alsa_handle, p_hw_params,
                                                    &adevice->current_audio_period_size,
                                                    0);
        if (alsa_err < 0)
        {
            ERROR_NORET ("[%d] Error setting period size", __LINE__);
            /* Free the hardware params then return error code */
            snd_pcm_hw_params_free (p_hw_params);
            return -1;
        }
#else
        alsa_err =
            snd_pcm_hw_params_set_period_size_near (p_alsa_handle, p_hw_params,
            		adevice->current_audio_period_size, 0);
        if (alsa_err < 0)
        {
            ERROR_NORET ("[%d] Error setting period size", __LINE__);
            /* Free the hardware params then return error code */
            snd_pcm_hw_params_free (p_hw_params);
            return -1;
        }
#endif
        if (tmp != adevice->current_audio_period_size)
        {
            WARNING
            ("[%d] requested period size not given (%d -> %d)",
             __LINE__, (int) tmp, (int) adevice->current_audio_period_size);
        }
        else
        {
            TRACE2("[%d] period_size set to %u\n", __LINE__,
                   (unsigned int) adevice->current_audio_period_size);
        }

        if (adevice->device_type == AUDIO_PCM_DEVICE)
        {
            /*
             * calculate the alsa_period based on audio_duration as sent from
             * application.
             */
            tmp_uint = AUDIO_PERIOD_DEFAULT;
#ifdef ALSA_PCM_NEW_HW_PARAMS_API
            alsa_err =
                snd_pcm_hw_params_set_periods_near (p_alsa_handle, p_hw_params,
                                                    &tmp_uint, 0);
            if (alsa_err < 0)
            {
                ERROR_NORET ("[%d]: Error setting periods.", __LINE__);
                /* Free the hardware params then return error code */
                snd_pcm_hw_params_free (p_hw_params);
                return -1;
            }
#else
            alsa_err =
                snd_pcm_hw_params_set_periods_near (p_alsa_handle, p_hw_params,
                                                    tmp_uint, 0);
            if (alsa_err < 0)
            {
                ERROR_NORET ("[%d] Error setting periods.", __LINE__);
                /* Free the hardware params then return error code */
                snd_pcm_hw_params_free (p_hw_params);
                return -1;
            }
#endif
            if (tmp_uint != AUDIO_PERIOD_DEFAULT)
            {
                WARNING
                ("[%d]  requested number of periods not given (%u -> %u)",
                 __LINE__, AUDIO_PERIOD_DEFAULT, tmp_uint);
            }
            else
            {
                TRACE2("[%d] Number of periods set to %u\n", __LINE__,
                		AUDIO_PERIOD_DEFAULT);
            }
        }
    }
    else                /* must be DEVICE_ALSA_AUDIO_NOT_USE_PERIODS */
    {
        /*
         * Using parameters to size the buffer based on how many bytes there is in
         * a period multiplied by the number of periods. There may be a better
         * way of doing this....
         */
        alsa_err =
            snd_pcm_hw_params_set_buffer_size (p_alsa_handle, p_hw_params,
                                               adevice->current_audio_blocksize_bytes *
                                               AUDIO_PERIOD_DEFAULT);
        if (alsa_err < 0)
        {
            ERROR_NORET ("[%d] Error setting buffersize.", __LINE__);
            /* Free the hardware params then return error code */
            snd_pcm_hw_params_free (p_hw_params);
            return -1;
        }
    }

    /*
     * Make it so! This call actually sets the parameters
    */
    alsa_err = snd_pcm_hw_params (p_alsa_handle, p_hw_params);
    if (alsa_err < 0)
    {
        ERROR_NORET ("[%d] Cannot set parameters (%s)", __LINE__,
                     snd_strerror (alsa_err));
        /* Free the hardware params then return error code */
        snd_pcm_hw_params_free (p_hw_params);
        return -1;
    }

    /* Free hardware config structure */
    snd_pcm_hw_params_free (p_hw_params);
    TRACE2("[%s] Exit\n", __FUNCTION__);
    return 0;
}

/*
 * This function pokes the alsa interface to find how many bytes are in alsa
 * buffer to be read.
 * this function needs to be called to decide whether to call alsa_read or not.
 */
int alsa_read_ready (audio_device_info_t* adevice, unsigned int *pbytes_to_read)
{
    int result_code = 0;
    unsigned int data_available;
    int frames_to_read;
    snd_pcm_t * p_alsa_handle = adevice->alsa_handle;

    /* Check that the passed handle is legal */
    if ((p_alsa_handle == (snd_pcm_t *) - 1) || (p_alsa_handle == NULL))
    {
        ERROR (-1, "[%d] ERROR: Invalid handle %p", __LINE__, p_alsa_handle);
    }

    /* Get number of frames available for read */
    frames_to_read = snd_pcm_avail_update (p_alsa_handle);

    /*
    * Negative value indicates an error. Usually this is a 'broken pipe' error
    * which indicates an overrun of input. If we detect this then
    * we need to call 'snd_pcm_prepare' to clear the error
    */
    if (frames_to_read < 0)
    {
        if (frames_to_read == -EPIPE)
        {
            WARNING ("[%d] rda: orun", __LINE__);
            /* Kick input to clear error */
            frames_to_read = snd_pcm_prepare (p_alsa_handle);

            if ( frames_to_read < 0 ) {
                ERROR (-1, "[%d]: Unable to recover", __LINE__);
                return -1;
            }
            frames_to_read = 0;

            /* Starting device after prepare. Alsa api snd_pcm_avail_update works only if device is in running state*/
            snd_pcm_state_t th;
            th = snd_pcm_state(p_alsa_handle);
            if ( th == SND_PCM_STATE_PREPARED ) {
                int err = snd_pcm_start (p_alsa_handle);
                CHECK_ERROR(err < 0, -1, "[%d] Cannot restart PCM (%s)", __LINE__,
                            snd_strerror (err));
                return -1;
            } else {
                WARNING ("[%d] unable to recover from orun case", __LINE__);
                return -1;
            }
        }
        else
        {
            WARNING ("[%d] %s", __LINE__, snd_strerror (frames_to_read));
            frames_to_read = 0;
        }
        result_code = -1;
    }

    /* Update number of bytes to read back to caller */
    data_available = frames_to_read * adevice->current_audio_framesize;
    /* Return the number of bytes available to read even if there isn't enough for a complete frame */
    *pbytes_to_read = data_available;

    return result_code;
}

/*
 * This function does interleaved read of the requested bytes from alsa buffer.
 *
 */
int alsa_read (audio_device_info_t* adevice, void *p_data_buffer, int data_length)
{
    int result_code = 0;
    int frames_read = 0;
    snd_pcm_t * p_alsa_handle = adevice->alsa_handle;

    /* Check that the passed handle is legal */
    if ((p_alsa_handle == (snd_pcm_t *) - 1) || (p_alsa_handle == NULL))
    {
        ERROR (-1, "[%d] Invalid handle %p", __LINE__, p_alsa_handle);
    }

    /* Check the passed buffer to see if it is what is needed */
    if (p_data_buffer == NULL)
    {
        ERROR (-1, "[%d]: bad data buffer: input data address %p",
               __LINE__, p_data_buffer);
    }

    /* Now perform the ALSA read
    * ALSA works in 'frames' so we ask for the
    * 'number of bytes requested'/framesize. We then have to adjust the
    * number of frames actually read back to bytes
    */

    frames_read =
        snd_pcm_readi (p_alsa_handle, p_data_buffer,
                       (data_length / adevice->current_audio_framesize));
    if (frames_read < 0)
    {
        WARNING ("[%d] Read error: %s", __LINE__,
                 snd_strerror (frames_read));

        /* If we get a 'broken pipe' this is an overrun */
        /* Kick input to clear error */
        if (frames_read == -EPIPE)
            snd_pcm_prepare (p_alsa_handle);

        if (frames_read == -ENODEV)
            result_code = -2;
        else
            /* default error code */
            result_code = -1;
    }
    else
    {
        /* Read was OK so record the size (in bytes) of data read */
        return (frames_read * adevice->current_audio_framesize);
    }

    return result_code;
}

#if 0
/*
 * This function is used to get the full Alsa device name
 * from the user provided name
 */
static int
get_alsa_device_name (char **name, char * dev_name)
{
    static char getting=0;
    static void **hints, **n;
    int ret_val = -1;

    if (getting == 0)
    {
        if (snd_device_name_hint (-1, "pcm", &hints) < 0)
        {
            ERROR(-1, "[%d] Getting alsa device name failed", __LINE__);
        }
        n = hints;
    }

    while (*n != NULL)
    {
        *name = snd_device_name_get_hint (*n, "NAME");
        n++;
        if (strstr(*name,dev_name))
        {
            if (strstr(*name, "front"))
            {
                ret_val = 0;
                break;
            }
        }
    }

    snd_device_name_free_hint (hints);
    return ret_val;
}
#endif

/*
 * This function inits the alsa mixer interface for the already opened alsa
 * device. the handler returned from this function is used for
 * set_volume , mute and Unmute of the Mic.
 *
 */
int alsa_mixer_init(char* device_name)
{
    if (alsa_mixer_handle == NULL)
    {
        int err;
        /* open an handle to the mixer interface of alsa */
        err = snd_mixer_open(&alsa_mixer_handle, 0);
        CHECK_ERROR(err < 0, -1,
                    "Error calling snd_mixer_open: %s", snd_strerror(err));

        card_number = snd_card_get_index(device_name);
        CHECK_ERROR(card_number < 0, -1,
                    "Unable to get the card number for device '%s'.",
                    device_name);

        char mixer_device[64];
        sprintf(mixer_device, "hw:%d",card_number);

        /* attach the mixer interface to the audio device we are in interested in */
        err = snd_mixer_attach(alsa_mixer_handle, mixer_device);
        if (err < 0)
        {
            ERROR_NORET("Error calling snd_mixer_attach for device %s: %s", mixer_device, snd_strerror(err));
            return -1; //ERROR_SYSTEM_CALL
        }

        TRACE2("[%d] Attached mixer to %s\n", __LINE__, mixer_device);

        /* register the mixer simple element (selem) interface */
        err = snd_mixer_selem_register(alsa_mixer_handle, NULL, NULL);
        if (err < 0)
        {
            ERROR_NORET("Error calling snd_mixer_selem_register: %s", snd_strerror(err));
            return -1; //ERROR_SYSTEM_CALL
        }


        /* load the mixer elements */
        err = snd_mixer_load(alsa_mixer_handle);
        if (err < 0)
        {
            ERROR_NORET("Error calling snd_mixer_load: %s", snd_strerror(err));
            return -1;//ERROR_SYSTEM_CALL
        }

        /* find the selem handle, this allows use to use the
           mixer simple element interface */
        snd_mixer_selem_id_t *selemId = (snd_mixer_selem_id_t *) alloca(snd_mixer_selem_id_sizeof());
        memset(selemId, 0, snd_mixer_selem_id_sizeof());

        const char** ptable;
        unsigned int ptable_len;
        ptable = mixer_mic_table;
        ptable_len = sizeof(mixer_mic_table)/sizeof(mixer_mic_table[0]);
        char selem_name[64];
        int found_selem = 0;
        snd_mixer_elem_t *elem;
        unsigned int i;

        for (i=0; i<ptable_len; i++)
        {
            /* look through all the elements looking for one we can use */
            for (elem = snd_mixer_first_elem(alsa_mixer_handle); elem; elem = snd_mixer_elem_next(elem))
            {
                snd_mixer_selem_get_id(elem, selemId);
                strcpy(selem_name, snd_mixer_selem_id_get_name(selemId));
                /* check if the element is active */
                if (snd_mixer_selem_is_active(elem) == 0)
                {
                    TRACE2("[%d] Ignoring inactive element '%s'\n", __LINE__, selem_name);
                    continue;
                }
                if (strcmp(selem_name, ptable[i]) == 0)
                {
                    TRACE2("[%d] Using element '%s'\n",  __LINE__, selem_name);
                    mixer_elem = elem;
                    found_selem = 1;
                    break;
                }
            }
            if (found_selem == 1)
            {
                /* we found a match so continue */
                break;
            }
        }

        if (found_selem == 0)
        {
            ERROR_NORET ("Error failed to find a mixer element to control device = %s",mixer_device);
            snd_mixer_close(alsa_mixer_handle);
            alsa_mixer_handle = NULL;
            return -1; //ERROR_SYSTEM_CALL
        }

        /* the gain in the api is in the range 0 to 100.  Convert the gain
         * from the range in the api to the range the sound card expects
         * */
        err = snd_mixer_selem_get_capture_volume_range(mixer_elem, &alsa_mixer_min_volume, &alsa_mixer_max_volume);
        if (err < 0)
        {
            ERROR(-1, "Error calling snd_mixer_selem_get_capture_volume_range: %s",snd_strerror(err));
        }

        /* make sure Mic is not muted */
        err = mxuvc_audio_set_mic_mute(0);
        if (err < 0)
        {
            ERROR(-1," Unable to unmute the Mic. err = %d", err);
        }


    }

    return 0; //SUCCESS;
}

/*
 * This function frees all resources allocated.
 * */

void free_resources()
{
    if (alsa_mixer_handle)
    {
        snd_mixer_close(alsa_mixer_handle);
        alsa_mixer_handle = NULL;
        mixer_elem = NULL;
    }

    if (audio_device_name)
    {
        free(audio_device_name);
        audio_device_name = NULL;
    }
}

/*
 * This function is used to get the full Alsa device informations
 * from the user provided name
 */
int get_alsa_card_info (char * dev_name, int *cardnum, int *devcount)
{
    snd_ctl_t *handle;
    int card, err, dev, count = 0;
    int ret = 0;

    card = -1;
    if (snd_card_next(&card) < 0) {
        TRACE("no soundcards found...\n");
        return 1;
    }
    card = snd_card_get_index(dev_name);

    if (card>=0) {
        char name[32];
        sprintf(name,"hw:%d",card);

        if ((err = snd_ctl_open(&handle, name, 0)) < 0) {
            TRACE("error snd_ctl_open\n");
            ret = 1;
            return ret;
        }
        dev = -1;
        while (1) {
            if (snd_ctl_pcm_next_device(handle, &dev)<0)
                TRACE("err: snd_ctl_pcm_next_device\n");
            if (dev < 0) {
                break;
            }

            count++;
        }
        snd_ctl_close(handle);
    }
    *cardnum = card;
    *devcount = count;

    return ret;
}

/* This function is used to get the alsa hctl device num id on which mute 
 * function has to act
 */
int get_alsa_hctl_numid(int cardno,int values)
{
    int err;
    char card[10];
    snd_hctl_t *handle;
    snd_hctl_elem_t *elem;
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);

    sprintf(card, "hw:%d", cardno);

    if ((err = snd_hctl_open(&handle, card, 0)) < 0) {
        ERROR( -1, "Control %s open error: %s", card, snd_strerror(err));
    }

    if ((err = snd_hctl_load(handle)) < 0) {
        ERROR(-1,"Control %s local error: %s\n", card, snd_strerror(err));
    }

    for (elem = snd_hctl_first_elem(handle); elem; elem = snd_hctl_elem_next(elem)) {
        if ((err = snd_hctl_elem_info(elem, info)) < 0) {
            snd_hctl_close(handle);
            ERROR(-1,"Control %s snd_hctl_elem_info error: %s\n", card, snd_strerror(err));
        }

        if (snd_ctl_elem_info_is_inactive(info))
            continue;

        const char *str = (const char *)snd_ctl_elem_info_get_name(info);

        int count = snd_ctl_elem_info_get_count(info);

        if ( (strcmp(str,"Mic Capture Switch") == 0) && (count == values)){

            int numid = snd_ctl_elem_info_get_numid(info);
            if ( numid < 0 )
                ERROR_NORET("ERROR finding the correct numid for mute setting %s\n",card);

            snd_hctl_close(handle);
            return numid;
        }
            
    }

    ERROR_NORET("ERROR unable to find the correct numid for mute setting %s\n",card);

    if ( handle )
        snd_hctl_close(handle);

    return -1;
}

/* this funtion will set the required funtionality given by cmd
 *  on alsa card specified by cardnum and numid
 */
int set_alsa_hctl_func(int numid, char *cmd, int cardnum)
{

    int err = -1;
    char card[64];
    char num_id[20];

    /* control handle params */
    snd_ctl_t *alsa_ctl_handle = NULL;
    snd_ctl_elem_info_t *alsa_ctl_info;
    snd_ctl_elem_id_t *alsa_ctl_id;
    snd_ctl_elem_value_t *alsa_ctl_control;

    /* Initialize the variables */
    snd_ctl_elem_info_alloca(&alsa_ctl_info);
    snd_ctl_elem_id_alloca(&alsa_ctl_id);
    snd_ctl_elem_value_alloca(&alsa_ctl_control);

    sprintf(card, "hw:%d", cardnum);

    sprintf(num_id, "numid=%d",numid);


    if (snd_ctl_ascii_elem_id_parse(alsa_ctl_id, num_id)) 
    {
        ERROR(-1, "Wrong control identifier: %d\n",numid);
    }
    
    if (alsa_ctl_handle == NULL && (err = snd_ctl_open(&alsa_ctl_handle, card, 0)) < 0) 
    {
        ERROR(-1,"Control %s open error: %s\n", card, snd_strerror(err));
    }

    snd_ctl_elem_info_set_id(alsa_ctl_info, alsa_ctl_id);

    if ((err = snd_ctl_elem_info(alsa_ctl_handle, alsa_ctl_info)) < 0) 
    {
        snd_ctl_close(alsa_ctl_handle);
        ERROR(-1,"Cannot find the given element from control %s\n", card);
    }

    snd_ctl_elem_info_get_id(alsa_ctl_info, alsa_ctl_id);

    snd_ctl_elem_value_set_id(alsa_ctl_control, alsa_ctl_id);

    if ((err = snd_ctl_elem_read(alsa_ctl_handle, alsa_ctl_control)) < 0) 
    {
        snd_ctl_close(alsa_ctl_handle);
        ERROR(-1,"Cannot read the given element from control\n");
    }

    err = snd_ctl_ascii_value_parse(alsa_ctl_handle, alsa_ctl_control, alsa_ctl_info, cmd);
    if (err < 0) 
    {
        snd_ctl_close(alsa_ctl_handle);
        ERROR(-1,"Control parse error: %s\n", snd_strerror(err));
    }

    if ((err = snd_ctl_elem_write(alsa_ctl_handle, alsa_ctl_control)) < 0) 
    {
        snd_ctl_close(alsa_ctl_handle);
        ERROR(-1,"Control element write error: %s\n", snd_strerror(err));
    }

    if ( alsa_ctl_handle )
    {
        snd_ctl_close(alsa_ctl_handle);
        alsa_ctl_handle = NULL;
    }

    return 0;
}

