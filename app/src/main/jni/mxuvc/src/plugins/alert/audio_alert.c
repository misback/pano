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
#include <libusb-1.0/libusb.h>
#include <math.h>
#include "libusb/handle_events.h"
#include "mxuvc.h"
#include "alert.h"
#include "common.h"

#define AUDIO_THRESHOLD_DB_DEFAULT 100

//static variables
static int alert_enable = 0;
static unsigned int audio_threshold_dB = AUDIO_THRESHOLD_DB_DEFAULT;
static void *cb_auser_data;

mxuvc_audio_alert_cb_t alert_audio_callback;

//void (*alert_audio_callback)(void *audalert_info, void *user_data);
//
int mxuvc_alert_audio_register_cb(mxuvc_audio_alert_cb_t func,void *user_data)
{

	alert_audio_callback = func;
	cb_auser_data = user_data;
	return 0;
}

int mxuvc_alert_audio_enable(void)
{
	int ret = 0;
	alert_type type;

	float PowVal;
	unsigned int aud_threshold_val;
	CHECK_ERROR(check_alert_state() != 1, -1,
			"alert module is not initialized");
		
	if(alert_enable==0)
		alert_enable = 1;

	type.d32 = 0;
	type.b.audio_alert = 1;
	//convert DB to Intensity
	PowVal = (float)(audio_threshold_dB/20.0);
	PowVal = pow(10, PowVal);	
	aud_threshold_val = (unsigned int)(PowVal + 0.5);

	ret = set_alert(type, ALARM_ACTION_ENABLE, aud_threshold_val, NULL, 0, 0, 0, NULL);
	CHECK_ERROR(ret != 0, -1, "set_alert failed");

	return ret;
}

int mxuvc_alert_audio_set_threshold(unsigned int audioThresholdDB)
{
    int ret = 0;
    alert_type type;

    float PowVal;
    unsigned int aud_threshold_val;
    CHECK_ERROR(check_alert_state() != 1, -1,
            "alert module is not initialized");

    audio_threshold_dB = audioThresholdDB;
    if(alert_enable==0)
        return ret;

    type.d32 = 0;
    type.b.audio_alert = 1;
    //convert DB to Intensity
    PowVal = (float)(audio_threshold_dB/20.0);
    PowVal = pow(10, PowVal);
    aud_threshold_val = (unsigned int)(PowVal + 0.5);

    ret = set_alert(type, ALARM_ACTION_ENABLE, aud_threshold_val, NULL, 0, 0, 0, NULL);
    CHECK_ERROR(ret != 0, -1, "set_alert failed");

    return ret;
}

/* api to disable audio alarm */
int mxuvc_alert_audio_disable(void)
{
	alert_type type;
	int ret = 0;

	CHECK_ERROR(alert_enable == 0, -1,
			"Audio alert module is not initialized");

	if(alert_enable){
		alert_enable = 0;
		type.d32 = 0;
		type.b.audio_alert = 1;

		ret = set_alert(type, ALARM_ACTION_DISABLE, 0, NULL, 0, 0, 0, NULL);
		CHECK_ERROR(ret != 0, -1, "set_alert failed");
	}

	return ret;
}

/* api to get current audio intensity */
int mxuvc_get_current_audio_intensity(unsigned int *audioIntensityDB)
{
	int ret = 0; 

	ret = get_audio_intensity(audioIntensityDB);

	CHECK_ERROR(ret != 0, -1,
			"mxuvc_get_current_audio_intensity failed");

	//convert intensity to db
	if(*audioIntensityDB >= 32767)
		return -1;
	
	/* check return value against zero as log10 gives undefined value for zero */
	*audioIntensityDB = (*audioIntensityDB == 0)? 0 : (20 * log10(*audioIntensityDB)) + 0.5;

	return ret;
}


int proces_audio_alert(audio_alert_info *aalert)
{
 	int audioIntensityDB = aalert->audioThresholdDB;

	//convert intensity to db
	if(audioIntensityDB >= 32767)
		return -1;

	/* check return value against zero as log10 gives undefined value for zero */
	audioIntensityDB = (audioIntensityDB == 0)? 0 :  (20 * log10(audioIntensityDB)) + 0.5;

	if(alert_enable && alert_audio_callback)
		alert_audio_callback(aalert, cb_auser_data);	

	return 0;
}
