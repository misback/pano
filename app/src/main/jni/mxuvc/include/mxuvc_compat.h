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

#ifndef __MXUVC_COMPAT_H__
#define __MXUVC_COMPAT_H__

#define mxuvc_register_video_cb(args...)	mxuvc_video_register_cb(args)
#define mxuvc_get_channel_count(args...)	mxuvc_video_get_channel_count(args)
#define mxuvc_set_resolution(args...)		mxuvc_video_set_resolution(args)
#define mxuvc_get_resolution(args...)		mxuvc_video_get_resolution(args)
#define mxuvc_set_video_format(args...)		mxuvc_video_set_format(args)
#define mxuvc_get_video_format(args...)		mxuvc_video_get_format(args)
#define mxuvc_force_iframe(args...)		mxuvc_video_force_iframe(args)
#define mxuvc_set_framerate(args...)		mxuvc_video_set_framerate(args)
#define mxuvc_get_framerate(args...)		mxuvc_video_get_framerate(args)
#define mxuvc_set_brightness(args...)		mxuvc_video_set_brightness(args)
#define mxuvc_get_brightness(args...)		mxuvc_video_get_brightness(args)
#define mxuvc_set_contrast(args...)		mxuvc_video_set_contrast(args)
#define mxuvc_get_contrast(args...)		mxuvc_video_get_contrast(args)
#define mxuvc_set_hue(args...)			mxuvc_video_set_hue(args)
#define mxuvc_get_hue(args...)			mxuvc_video_get_hue(args)
#define mxuvc_set_saturation(args...)		mxuvc_video_set_saturation(args)
#define mxuvc_get_saturation(args...)		mxuvc_video_get_saturation(args)
#define mxuvc_set_gain(args...)			mxuvc_video_set_gain(args)
#define mxuvc_get_gain(args...)			mxuvc_video_get_gain(args)
#define mxuvc_set_zoom(args...)			mxuvc_video_set_zoom(args)
#define mxuvc_get_zoom(args...)			mxuvc_video_get_zoom(args)
#define mxuvc_get_pan(args...)			mxuvc_video_get_pan(args)
#define mxuvc_set_pan(args...)			mxuvc_video_set_pan(args)
#define mxuvc_get_tilt(args...)			mxuvc_video_get_tilt(args)
#define mxuvc_set_tilt(args...)			mxuvc_video_set_tilt(args)
#define mxuvc_get_pantilt(args...)		mxuvc_video_get_pantilt(args)
#define mxuvc_set_pantilt(args...)		mxuvc_video_set_pantilt(args)
#define mxuvc_set_gamma(args...)		mxuvc_video_set_gamma(args)
#define mxuvc_get_gamma(args...)		mxuvc_video_get_gamma(args)
#define mxuvc_set_sharpness(args...)		mxuvc_video_set_sharpness(args)
#define mxuvc_get_sharpness(args...)		mxuvc_video_get_sharpness(args)
#define mxuvc_set_bitrate(args...)		mxuvc_video_set_bitrate(args)
#define mxuvc_get_bitrate(args...)		mxuvc_video_get_bitrate(args)
#define mxuvc_set_goplen(args...)		mxuvc_video_set_goplen(args)
#define mxuvc_get_goplen(args...)		mxuvc_video_get_goplen(args)
#define mxuvc_set_profile(args...)		mxuvc_video_set_profile(args)
#define mxuvc_get_profile(args...)		mxuvc_video_get_profile(args)
#define mxuvc_set_maxnal(args...)		mxuvc_video_set_maxnal(args)
#define mxuvc_get_maxnal(args...)		mxuvc_video_get_maxnal(args)
#define mxuvc_set_flip_vertical(args...)	mxuvc_video_set_flip_vertical(args)
#define mxuvc_get_flip_vertical(args...)	mxuvc_video_get_flip_vertical(args)
#define mxuvc_set_flip_horizontal(args...)	mxuvc_video_set_flip_horizontal(args)
#define mxuvc_get_flip_horizontal(args...)	mxuvc_video_get_flip_horizontal(args)
#define mxuvc_set_wdr(args...)			mxuvc_video_set_wdr(args)
#define mxuvc_get_wdr(args...)			mxuvc_video_get_wdr(args)
#define mxuvc_set_exp(args...)			mxuvc_video_set_exp(args)
#define mxuvc_get_exp(args...)			mxuvc_video_get_exp(args)
#define mxuvc_set_zone_exp(args...)		mxuvc_video_set_zone_exp(args)
#define mxuvc_get_zone_exp(args...)		mxuvc_video_get_zone_exp(args)
#define mxuvc_set_max_framesize(args...)	mxuvc_video_set_max_framesize(args)
#define mxuvc_get_max_framesize(args...)	mxuvc_video_get_max_framesize(args)
#define mxuvc_set_max_analog_gain(args...)	mxuvc_video_set_max_analog_gain(args)
#define mxuvc_get_max_analog_gain(args...)	mxuvc_video_get_max_analog_gain(args)
#define mxuvc_set_histogram_eq(args...)		mxuvc_video_set_histogram_eq(args)
#define mxuvc_get_histogram_eq(args...)		mxuvc_video_get_histogram_eq(args)
#define mxuvc_set_sharpen_filter(args...)	mxuvc_video_set_sharpen_filter(args)
#define mxuvc_get_sharpen_filter(args...)	mxuvc_video_get_sharpen_filter(args)
#define mxuvc_set_gain_multiplier(args...)	mxuvc_video_set_gain_multiplier(args)
#define mxuvc_get_gain_multiplier(args...)	mxuvc_video_get_gain_multiplier(args)
#define mxuvc_register_audio_cb(args...)	mxuvc_audio_register_cb(args)
#define mxuvc_set_audio_samplerate(args...)	mxuvc_audio_set_samplerate(args)
#define mxuvc_get_audio_samplerate(args...)	mxuvc_audio_get_samplerate(args)
#define mxuvc_set_audio_volume(args...)		mxuvc_audio_set_volume(args)
#define mxuvc_get_audio_volume(args...)		mxuvc_audio_get_volume(args)
#define mxuvc_set_mic_mute(args...)		mxuvc_audio_set_mic_mute(args)
#define mxuvc_set_audio_format(args...)		mxuvc_audio_set_format(args)

#endif  // #ifdef __MXUVC_COMPAT_H__
