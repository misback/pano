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

#ifndef __MXUVC_H__
#define __MXUVC_H__

#include <stdint.h>
#include "mxuvc_dewarp.h"
#include <stdbool.h>
#include "libusb/handle_events.h"

/** @file
 * @brief mxuvc API
 */

#ifndef MXUVC_TRACE_LEVEL
#define MXUVC_TRACE_LEVEL 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup video_system Video Subsystem
  * @{
  */

/** enum of video format used in a particular channel */
typedef enum {
    FIRST_VID_FORMAT       = 0,
    /** H264 in elementary stream format */
    VID_FORMAT_H264_RAW    = 0,
    /** H264 in transport stream format */
    VID_FORMAT_H264_TS     = 1,
    /** MJPEG in elmentary stream format */
    VID_FORMAT_MJPEG_RAW   = 2,
    /** YUV stream in YUY2 format */
    VID_FORMAT_YUY2_RAW    = 3,
    /** YUV stream in NV12 format */
    VID_FORMAT_NV12_RAW    = 4,
    /** Luma stream format */
    VID_FORMAT_GREY_RAW    = 5,
    /** H264 and AAC in transport stream format */
    VID_FORMAT_H264_AAC_TS = 6,
    /** mux data */
    VID_FORMAT_MUX         = 7,
    /** datas */
    VID_FORMAT_METADATAS       = 8,
    /** total number of video formats supported */
    NUM_VID_FORMAT
} video_format_t;


/** enum to indicate the video channel number */
typedef enum channel {
    FIRST_VID_CHANNEL = 0,

    /** channels for skype mode */
    /** channel for main video */
    CH_MAIN    = 0,
    /** channel for preview video */ 
    CH_PREVIEW = 1,
    /** number of video channels in skype mode */   
    NUM_SKYPE_VID_CHANNELS,
    /** number of video channels in skype mode */   
    NUM_VID_CHANNEL = NUM_SKYPE_VID_CHANNELS,

    /** channels for ip camera */
    /** video channel1 */
    CH1  = 0,
    /** video channel2 */
    CH2  = 1,
    /** video channel3 */
    CH3  = 2,
    /** video channel4 */
    CH4  = 3,
    /** video channel5 */
    CH5  = 4,
    /** video channel6 */
    CH6  = 5,
    /** video channel7 */
    CH7  = 6,
    /** number of mux channels in ip camera */
    NUM_MUX_VID_CHANNELS  = CH7+1,
    /** video channel for RAW stream */
    CH_RAW            = NUM_MUX_VID_CHANNELS,
    /** total number of video channels in ip camera */
    NUM_IPCAM_VID_CHANNELS
} video_channel_t;

/** enum to indicate the H264 profile used for encoding */
typedef enum {
    /** baseline profile */
    PROFILE_BASELINE = 0,
    /** main profile */
    PROFILE_MAIN     = 1,
    /** high profile */
    PROFILE_HIGH     = 2,
    /** number of h264 profiles supported */
    NUM_PROFILE
} video_profile_t;

/** enum to turn on/off video flip */
typedef enum {
    /** turn off flip */
    FLIP_OFF    = 0,
    /** trun on flip */
    FLIP_ON     = 1,
    /** number of flip mode supported */
    NUM_FLIP
} video_flip_t;

/** enum to set the Wide Dynamic Range (WDR) mode */
typedef enum {
    /** disable WDR */
    WDR_DISABLE = 0,
    /** auto WDR */
    WDR_AUTO    = 1,
    /** manual WDR */
    WDR_MANUAL  = 2,
    /** number of WDR modes supported */
    NUM_WDR
} wdr_mode_t;

/** TBD */
typedef enum {
    /** TBD */
    HISTO_EQ_OFF = 0,
    /** TBD */
    HISTO_EQ_ON =  1,
    /** TBD */
    NUM_HISTO_EQ
} histo_eq_t;

typedef enum {
    SATURATION_AUTO = 0,
    SATURATION_MANUAL  = 1,
    NUM_SATURATION
} saturation_mode_t;

typedef enum {
    BRIGHTNESS_AUTO = 0,
    BRIGHTNESS_MANUAL  = 1,
    NUM_BRIGHTNESS
} brightness_mode_t;

typedef enum {
    CONTRAST_AUTO = 0,
    CONTRAST_MANUAL  = 1,
    NUM_CONTRAST
} contrast_mode_t;

/** compressed alpha download error code*/
typedef enum{
    /** success */
    COMPRESSED_ALPHA_SUCCESS = 0,
    /** firmware is busy processing the request from host */
    COMPRESSED_ALPHA_STATUS_BUSY = 1,
    /** firmware not ready to accept the compressed token */
    COMPRESSED_ALPHA_ERROR_TOKEN_NOT_READY = 2,
    /** error while allocating memory in firmware to hold the decompressed alpha map */
    COMPRESSED_ALPHA_ERROR_DECOMPRESSION_MALLOC = 3,
    /** error while decompressing the compressed alpha map in firmware */
    COMPRESSED_ALPHA_ERROR_DECOMPRESSION_FAILURE = 4,
    /** error while converting the raster format alpha map to tile format in firmware */
    COMPRESSED_ALPHA_ERROR_RASTER_TO_TILE = 5,
    /** error while allocating memory in firmware to receive compressed alpha map from host */
    COMPRESSED_ALPHA_ERROR_UVC_MALLOC = 6,
} compressed_alpha_errorcode_t;

/** Structure containing the information regarding an encoding channel */
typedef struct
{
    /** format of the video used in the channel */
    video_format_t  format;

    /** width of the video in the channel */
    uint16_t    width;

    /** height of the video in the channel */
    uint16_t    height;

    /** frame Rate of the video in the channel */
    uint32_t    framerate;

    /** GOP size of video to be used in the channel, applicable only for
    VID_FORMAT_H264_RAW,
    VID_FORMAT_H264_TS,
    VID_FORMAT_H264_AAC_TS */
    uint32_t    goplen;

    /** H264 profile used for encoding in the channel, applicable only for
    VID_FORMAT_H264_RAW,
    VID_FORMAT_H264_TS,
    VID_FORMAT_H264_AAC_TS */
    video_profile_t profile;

    /** bitrate of the video in the channel */
    uint32_t    bitrate;

    /** compression quality in terms of the QP factor set for the video
     on this channel, applicable only for VID_FORMAT_MJPEG_RAW */
    uint32_t    compression_quality;
} video_channel_info_t;

/** structure containing video crop information */
typedef struct {
    /** enable/disable cropping. Default: 0, Min: 0, Max: 1 */
    uint16_t enable;

    /** width to be cropped from the image. Default: 640, Min: 16, Max: 1920 */
    uint16_t width;

    /** height to be cropped from the image. Default: 480, Min: 16, Max: 1080 */
    uint16_t height;

    /** X offset from which the image is cropped. Default: 0, Min: 0, Max: 1920 */
    uint16_t x;

    /** Y Offset from which the image is cropped. Default: 0, Min: 0, Max: 1080 */
    uint16_t y;

} crop_info_t;


/** structure containing video crop information */
typedef struct {
    /** width of AE ROI region*/
    uint16_t width;

    /** height of AE ROI region */
    uint16_t height;

    /** X offset of AE ROI region relative to top left corner */
    uint16_t x;

    /** Y Offset of AE ROI region relative to top left corner */
    uint16_t y;

    /** Options for using the ROI */
    /*  0: Revert to default ISP ROI as set in JSON
        1: Use the passed ROI only (use weight 15 for ROI, 0 outside)
        2: Add the passed ROI (use weight 15 in ROI, keep outside weights same as before)
        3: Remove the passed ROI (use weight 0 for ROI, keep outside weights same as before) */
    uint16_t mode;

} isp_ae_roi_info_t;

/** structure containing the information regarding motion vector statistics */
typedef struct {
    /** buffer containing the motion stats */
    uint8_t *buf;
    /** size of the data in bytes */
    int size;
} motion_stat_t;

/** TBD */
typedef struct
{
    /** TBD */
    uint32_t    PMEVectorSize;
    /** TBD */
    uint32_t    PMEVectorType;
    /** TBD */
    uint32_t    mbWidth;
    /** TBD */
    uint32_t    mbHeight;
    /** TBD */
    uint32_t    PictStr;
    /** TBD */
    uint32_t    ResultsMV;
    /** TBD */
    uint32_t    MCost;
    /** TBD */
    uint32_t    NumRef;
    /** TBD */
    uint32_t    Refid0;
    /** TBD */
    uint32_t    Refid1;
    /** TBD */
    uint32_t    PMEMaxYRange;
} PMEVectorTag_t;

/** TBD */
typedef struct {
    /** TBD */
    PMEVectorTag_t *pmevtag;
    /** TBD */
    int8_t   *PMEVectorAddress;
} pme_info_t;

/** TBD */
typedef struct {
   /** TBD */
    uint8_t *buf;
    /** TBD */
    int size;
} rectangle_info_t;

/** Histogram Data */
typedef struct {
   /** Pointer to histogram data */
    uint8_t *buf;
    /** Size of histogram data */
    int size;
} histogram_info_t;

typedef struct {
/// \brief Reports total sum of all Luma pixels in the entire
    unsigned int GPictYSum;
/// \brief Reports total sum of all Chroma pixels in the entire
    unsigned int GPictCSum;
/// \brief Reports sum of horizontal high frequency Luma information for the entire picture
    unsigned int GPictYHorzHighFreq;
/// \brief Reports sum of vertical high frequency Luma information for the entire picture
    unsigned int GPictYVertHighFreq;
/// \brief Reports sum of vertical high frequency Chroma information for the entire picture
    unsigned int GPictCHighFreq;
/// \brief Counts how many macroblocks had any Luma high frequency information. 
///  The hardware averages horizontal and vertical Luma high frequencies for each macroblock.
///  This value reflects the size of the non-zero two-dimensional high frequency information.
    unsigned int GPictYHighFreqSize;
/// \brief Counts how many macroblocks had any Chroma high frequency information. 
///  The hardware detects chroma high frequency for each macroblock.
///  This value reflects the size of the non-zero high frequency information.
    unsigned int GPictCHighFreqSize;
/// \brief Counts how many macroblocks had any Luma spatial edge information. 
/// The only macroblocks not counted are those with null strength information.
/// This register reflects the size of the spatial Luma edge content of the picture.
    unsigned int GPictYEdgeSize;
/// \brief Counts how many macroblocks had any Luma Motion edge information.
/// The only macroblocks not counted are those without Luma Motion direction and strength information. 
/// \brief Provides the sum of MB averages for all macroblock luma edge strengths in the picture. 
    unsigned int GPictEdgeStrengthMBSum;
/// \brief Total sum of pixels classified as having Color[0-3].
    unsigned int GPictColorSum[4];
/// \brief Counts how many macroblocks had any Color[0-3] information. 
/// The only macroblocks not counted are those without Color[0-3] information. 
/// This register reflects the size of areas of the picture in Color[0-3].
    unsigned int GPictColorSize[4];
} global_stats_t;


/** Global VPP Stats Data */
typedef struct {
   /** Pointer to VPP global stats data */
    global_stats_t *buf;
    /** Size of global stats data */
    int size;
} globalstats_info_t;

/** QMED extension */
typedef struct {
  /** Pointer to QMED extension data */
    char *qmedExt;
   /** Size of QMED extension data */
    unsigned int total_len;
} metadata_t;

/** QMED */
typedef struct {
  /** Pointer to QMED data */
    char *qmedPtr;
   /** Size of QMED data */
    unsigned int total_len;
} qmed_t;


/** Video Information structure used for processing the video data received 
from the camera in the callback function */
typedef struct {
    /** format of the video frame received */
    video_format_t format;

    /** video frame timestamp in terms of ticks of 90kHz clock, where each tick
     corresponds to 1/(90 * 1000) sec or 1/90 ms */
    uint64_t       ts;

    /** motion vector statistics information of the video frame,
     only in case of
     VID_FORMAT_YUY2_RAW,
     VID_FORMAT_NV12_RAW
     VID_FORMAT_GREY_RAW */
    motion_stat_t  stats;

    /** physical buffer index of the video frame dequeued by the V4L.
     This needs to be used by MXUVC application to queue back the video frame
     after processing, in the mxuvc_video_cb_buf_done() function described
     later */
    int            buf_index;

    /** PME data */
    pme_info_t     pme;
    /** rectangle data when smart motion is running on xtensa */
    rectangle_info_t  rect;
    /** Histogram data */
    histogram_info_t histogram;
    /** VPP global stats data */
    globalstats_info_t globalstats;
    /** Metadata */
    metadata_t     metadata;
    /** QMed */
    qmed_t qmed;
}__attribute__((packed))video_info_t;

/** enum to set the noise filter mode for the image processing */
typedef enum {
    /** auto mode */
    NF_MODE_AUTO = 0,
    /** manual mode */
    NF_MODE_MANUAL = 1,
    /** number of noise filter modes supported */
    NUM_NF
} noise_filter_mode_t;

typedef struct {
    uint16_t mode       ; // 0 : Bypass  1: Adpative_with_sensor_defaults  2:Adaptive_with_api_params  3:Manual
    uint16_t minNR      ;
    uint16_t maxNR      ;
    uint16_t minThresh  ;
    uint16_t maxThresh  ;
    uint16_t triggerPt  ;
}sinter_info_t;

/** enum to set the auto white balance params */
typedef struct {
    /** red gain */
	uint32_t redGain;
    /** blue gain */
	uint32_t blueGain;
    /** green gain */
	uint32_t greenGain;
    /** red green ratio */
	uint32_t redGreenRatio;
    /** blue green ratio */
	uint32_t blueGreenRatio;
}awb_params_t;

/** enum to enable/disable sensor zonal white balance */
typedef enum {
    /** disbale zonal white balance */
    ZONE_WB_DISABLE = 0,
    /** enable zonal white balance */
    ZONE_WB_ENABLE  = 1,
    /** number of zonal white balance modes supported */
    NUM_ZONE_WB,
}zone_wb_set_t;

/** enum to set power line frequency mode */
typedef enum {
    /** disable power line frequency */
    PWR_LINE_FREQ_MODE_DISABLE = 0,
    /** 50Hz power line frequency */
    PWR_LINE_FREQ_MODE_50HZ = 1,
    /** 60Hz power line frequency */
    PWR_LINE_FREQ_MODE_60HZ = 2
}pwr_line_freq_mode_t;

typedef enum {
    IPCAM = 0,
    SKYPE = 1,
    UNKNOWN
}camer_mode_t;

typedef struct {
    uint32_t minBitrate;
} vbr_params_t;

typedef enum mxuvc_error {
    /** Success (no error) */
    MXUVC_SUCCESS = 0,
    /** Input/output error */
    MXUVC_ERROR_IO = -1,
    /** Invalid parameter */
    MXUVC_ERROR_INVALID_PARAM = -2,
    /** Access denied */
    MXUVC_ERROR_ACCESS = -3,
    /** No such device */
    MXUVC_ERROR_NO_DEVICE = -4,
    /** Entity not found */
    MXUVC_ERROR_NOT_FOUND = -5,
    /** Resource busy */
    MXUVC_ERROR_BUSY = -6,
    /** Operation timed out */
    MXUVC_ERROR_TIMEOUT = -7,
    /** Overflow */
    MXUVC_ERROR_OVERFLOW = -8,
    /** Pipe error */
    MXUVC_ERROR_PIPE = -9,
    /** System call interrupted */
    MXUVC_ERROR_INTERRUPTED = -10,
    /** Insufficient memory */
    MXUVC_ERROR_NO_MEM = -11,
    /** Operation not supported */
    MXUVC_ERROR_NOT_SUPPORTED = -12,
    /** Device is not UVC-compliant */
    MXUVC_ERROR_INVALID_DEVICE = -50,
    /** Mode not supported */
    MXUVC_ERROR_INVALID_MODE = -51,
    /** Resource has a callback (can't use polling and async) */
    MXUVC_ERROR_CALLBACK_EXISTS = -52,
    /** Undefined error */
    MXUVC_ERROR_OTHER = -99
} mxuvc_error_t;

/** @defgroup video_init_deinit Init / Deinit
  * @{
  */

/**
 * @brief This API is used to initialize the Linux video interface on the Host
 * system.
 * 
 * @param[in] backend string representing the video backend. It is "v4l2" in
 *            this case
 * @param[in] options semi-colon separated list of options. Following options
 *            can be specified with this parameter:<br><br>
 *            <b>dev_offset</b> starting offset of the GEO camera video device node.
 *            If the camera is recognized as /dev/video0 and /dev/video1 then
 *            dev_offset should be equal to 0. If it is recognized as /dev/video1
 *            and /dev/video2 (because there is another camera in /dev/video0)
 *            then dev_offset should be equal to 1, etc. This option is not
 *            mandatory. The default value is 0.<br><br>
 *            <b>dev_offset_secondary</b> offset of second video node (for raw capture).
 *            If the camera is recognized as /dev/video1 and /dev/video2 then
 *            dev_offset_secondary should be 2. This option can be used in cases
 *            where device numbers are not fixed and may change when devices are 
 *            reconnected. This option is not mandatory and will default to
 *            dev_offset+1.<br><br>
 *            <b>v4l_buffers</b> Number of memory mapped buffers for first video device. This
 *            option is also not mandatory. The default value is 8.
 *            <b>v4l_buffers_raw</b> Number of memory mapped buffers for second raw video channel. This
 *            option is also not mandatory. The default value is 8.
 * 
 * @return 0 on Success, -1 on Failure
 * 
 * Example Usage:
 * @code
 * mxuvc_video_init("v4l2", "dev_offset=0;v4l_buffers=16");
 * or
 * mxuvc_video_init("v4l2", "");
 * @endcode
 */

int mxuvc_video_init(const char *backend, const char *options, int fd);

/**
 * @brief This API is used to free all the memory allocated by the MXUVC.
 * This function automatically calls @ref mxuvc_video_stop if the video
 * has not been stopped yet, it is therefore not necessary to explicitly call
 * @ref mxuvc_video_stop before calling @ref mxuvc_video_deinit.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_deinit();

/**@}*/

/** @defgroup video_register Registration Of Video Callback
  * @{
  */

/**
 * @brief Video callback function through which mxuvc pass the captured video
 * frame to caller as and when the data available. 
 * 
 * @param[out] buffer buffer containing the video frame
 * @param[out] size   size of the video frame
 * @param[out] info   structure containing video metadata
 * @param[out] user_data pointer to user data supplied during registration
 *  
 */
typedef void (*mxuvc_video_cb_t)(unsigned char *buffer, unsigned int size,
                                    video_info_t info, void *user_data);

/**
 * @brief This API is used to register the call back function with the MXUVC.
 * MXUVC calls this user function when the video data is available from the camera.
 * The Application can use this function to process the video data received from
 * the camera.
 * 
 * @param[in] ch        video channel on which the call back function is registered
 * @param[in] func      pointer to the call back function to be registered with
 *                      mxuvc, described below.
 * @param[in] user_data pointer to mxuvc user handle to be used for further processing,
 *                      when the callback function is called by the mxuvc
 *                      
 * @return 0 on Success, -1 on Failure
 * 
 * An example declaration of the call back function is as below.
 * 
 * @code
 * 
 * void example_cb(unsigned char *buffer, unsigned int size,video_info_t info, void *user_data)
 * 
 * Arguments:
 * buffer       pointer to the video data received from the camera
 * size         size of the video data received from the camera.
 * user_data    pointer to mxuvc user handle set when the call back function is registered.
 * info         information regarding the video frame, received from the camera.
 * 
 * @endcode
 * 
 */
int mxuvc_video_register_cb(video_channel_t ch, mxuvc_video_cb_t func,
                            void *user_data);

/**
 * @brief This API must be called after processing the data buffer received in the
 * callback @ref mxuvc_video_cb_t. When user call this api, mxuvc will release the buffer
 * and put back into data queue to fetch new data.
 * 
 * @param[in] ch        video channel on which the buffer needs to be returned
 *                      back to the mxuvc
 *                      
 * @param[in] buf_index physical buffer index returned as part video information
 *                      structure in the callback function @ref mxuvc_video_cb_t.
 *                      This is used to queue the buffer back to the V4L in the mxuvc.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_cb_buf_done(video_channel_t ch, int buf_index);

/**@}*/

/** @defgroup video_start_stop Start/Stop Video Capture
  * @{
  */

/**
 * @brief This API is used to start the capture of video data from the specified
 * channel in the camera.
 * 
 * @param[in] ch video channel on which the video capture needs to be started
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_start(video_channel_t ch);

/**
 * @brief This API is used to stop the capture of video data from a channel in
 * the camera.
 * 
 * @param[in] ch video channel on which the video capture needs to be stopped
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_stop(video_channel_t ch);

/**
 * @brief This API is used to check if the camera is active. It can be periodically
 * called to check the availability of the camera.
 * This function returns an error as not alive in the following conditions:
 * @li the camera has been unplugged
 * @li the camera or the USB host is no longer responding
 * 
 * @return 0 if Not alive, 1 if alive
 */
int mxuvc_video_alive();

/**@}*/

/** @defgroup video_control Video Parameters (Set/Get)
  * @{
  */

/**
 * @brief This API is used to get the number of video channels supported by the
 * GEO Camera. The number of video channels supported by the camera is configuration
 * dependent.
 * 
 * @param[out] count number of video channels supported. This parameter is returned
 *                   from the camera based on the configuration in which it is running.
 *                   
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_channel_count(uint32_t *count);

/**
 * @brief This API gets the information regarding the video parameters set on the
 * channel in the Camera.
 * 
 * @param[in]  ch    video channel from which the information is needed
 * @param[out] info  pointer to the video channel information structure returned
 *                   from the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_channel_info(video_channel_t ch, video_channel_info_t *info);

/**
 * @brief This API is used to force an I frame in the specified video channel.
 * Applicable only for VID_FORMAT_H264_RAW, VID_FORMAT_H264_TS, VID_FORMAT_H264_AAC_TS.
 * 
 * @param[in] ch video channel on which I frame needs to be forced
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_force_iframe(video_channel_t ch);

/**
 * @brief This API is used to get the format of the video data on the specified channel.
 * 
 * @param[in]   ch  video channel on which the format is queried
 * @param[out]  fmt pointer to receive the format of the video
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_format(video_channel_t ch, video_format_t *fmt);

/**
 * @brief This API is used to set the resolution of the video data on the specified channel.
 * 
 * @param[in] ch        video channel on which the resolution is to set
 * @param[in] width     width of the video in the specified channel
 * @param[in] height    height of the video in the specified channel
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_resolution(video_channel_t ch, uint16_t width, uint16_t height);

/**
 * @brief This API is used to get the resolution of the video data on the specified channel.
 * 
 * @param[in]  ch        video channel on which the resolution is queried
 * @param[out] width     pointer to receive the width of the video
 * @param[out] height    pointer to receive the height of the video
 * 
 * @return 0 on Success, -1 on Failure
 */
 int mxuvc_video_get_resolution(video_channel_t ch, uint16_t *width, uint16_t *height);

/**
 * @brief This API is used to set the video crop paramters on the specified channel.
 * 
 * @param[in] ch    video channel on which the crop information is to set
 * @param[in] info  video crop parameters to set on the channel
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_crop(video_channel_t ch, crop_info_t *info);

/**
 * @brief This API is used to get the video crop paramters of the specified channel.
 * 
 * @param[in]  ch    video channel on which the crop information is to be queried
 * @param[out] info  pointer to receive the video crop parameters
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_crop(video_channel_t ch, crop_info_t *info);

/**
 * @brief This api is used to set the frame rate on the video channel specified.
 * Typical Range: 1 to 30.
 * 
 * Not applicable for VID_FORMAT_YUY2_RAW,VID_FORMAT_NV12_RAW,VID_FORMAT_GREY_RAW.
 * 
 * @param[in] ch        video channel for which framerate is to set
 * @param[in] framerate framerate to set on the channel
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_framerate(video_channel_t ch, uint32_t framerate);

/**
 * @brief This api is used to get the frame rate on the video channel specified.
 * 
 * Not applicable for VID_FORMAT_YUY2_RAW,VID_FORMAT_NV12_RAW,VID_FORMAT_GREY_RAW.
 * 
 * @param[in]  ch        video channel for which framerate is to get
 * @param[out] framerate pointer to receive the framerate
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_framerate(video_channel_t ch, uint32_t *framerate);

/**
 * @brief This api is used to set GOP length on the video channel specified.
 * Typical Range: 0 to Max Integer (2147483647).
 * 
 * Applicable only for VID_FORMAT_H264_RAW, VID_FORMAT_H264_TS, VID_FORMAT_H264_AAC_TS.
 * 
 * @param[in] ch     video channel for which GOP length is to set
 * @param[in] value  GOP length to set on the channel
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_goplen(video_channel_t ch, uint32_t value);

/**
 * @brief This api is used to get GOP length on the video channel specified.
 * 
 * Applicable only for VID_FORMAT_H264_RAW, VID_FORMAT_H264_TS, VID_FORMAT_H264_AAC_TS.
 * 
 * @param[in]   ch      video channel for which GOP length is to get
 * @param[out]  value   pointer to receive the GOP length
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_goplen(video_channel_t ch, uint32_t *value);

/**
 * @brief This api is used to set bitrate on the video channel specified.
 * Typical Range: 100000 (100 kbps) to 2000000 (2 Mbps)
 * 
 * Not applicable for VID_FORMAT_YUY2_RAW,VID_FORMAT_NV12_RAW,VID_FORMAT_GREY_RAW.
 * 
 * @param[in] ch     video channel for which bitrate is to set
 * @param[in] value  bitrate to set on the channel
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_bitrate(video_channel_t ch, uint32_t value);

/**
 * @brief This api is used to get bitrate on the video channel specified.
 * 
 * Not applicable for VID_FORMAT_YUY2_RAW,VID_FORMAT_NV12_RAW,VID_FORMAT_GREY_RAW.
 * 
 * @param[in]  ch     video channel for which bitrate is to get
 * @param[out] value  pointer to receive the bitrate
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_bitrate(video_channel_t ch, uint32_t *value);

/**
 * @brief This api is used to set H264 profile on the video channel specified.
 * Values Supported: PROFILE_BASELINE, PROFILE_MAIN, PROFILE_HIGH. (See 2.1.2)
 * 
 * Applicable only for VID_FORMAT_H264_RAW, VID_FORMAT_H264_TS, VID_FORMAT_H264_AAC_TS.
 * 
 * @param[in] ch        video channel for which H264 profile is to set
 * @param[in] profile   H264 profile to set on the channel
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_profile(video_channel_t ch, video_profile_t profile);

/**
 * @brief This api is used to get H264 profile on the video channel specified.
 * 
 * Applicable only for VID_FORMAT_H264_RAW, VID_FORMAT_H264_TS, VID_FORMAT_H264_AAC_TS.
 * 
 * @param[in]   ch      video channel for which H264 profile is to get
 * @param[out]  profile pointer to receive the H264 profile info
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_profile(video_channel_t ch, video_profile_t *profile);

/**
 * @brief This api is used to set maximum size of the NAL unit received from the camera.
 * If this parameter is set to 0 the camera sends variable size NAL units. If it is set
 * to finite value the camera splits the frame into multiple equal size NAL units
 * with the maximum size equal to the parameter value. Applicable only for
 * VID_FORMAT_H264_RAW, VID_FORMAT_H264_TS, VID_FORMAT_H264_AAC_TS.
 * 
 * Typical Range: 0 to 2000.
 * 
 * @param[in] ch     video channel for which size of the NAL unit is to set
 * @param[in] value  size of the NAL unit to set on the channel
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_maxnal(video_channel_t ch, uint32_t value);

/**
 * @brief This api is used to get size of the NAL unit received from the camera.
 * Applicable only for VID_FORMAT_H264_RAW, VID_FORMAT_H264_TS, VID_FORMAT_H264_AAC_TS.
 * 
 * @param[in]  ch     video channel for which size of the NAL unit is to get
 * @param[out] value  pointer to receive the size of the NAL unit
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_maxnal(video_channel_t ch, uint32_t *value);

/**
 * @brief This api is used to set compression quality in terms of image
 * quantization parameter (QP). Applicable only for VID_FORMAT_MJPEG_RAW.
 * 
 * Typical Range: 0 to 10000.
 * 
 * @param[in] ch     video channel for which compression quality is to set
 * @param[in] value  compression quality to set on the channel
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_compression_quality(video_channel_t ch, uint32_t value);

/**
 * @brief This api is used to get compression quality in terms of image
 * quantization parameter (QP). Applicable only for VID_FORMAT_MJPEG_RAW.
 * 
 * @param[in]  ch     video channel for which compression quality is to get
 * @param[out] value  pointer to receive the compression quality
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_compression_quality(video_channel_t ch, uint32_t *value);

/**
 * @brief This api is used to set AVC/H264 level on the video channel.
 * Applicable only for VID_FORMAT_H264_RAW, VID_FORMAT_H264_TS,
 * VID_FORMAT_H264_AAC_TS.
 * 
 * Typical Range: 10 to 52
 * 
 * @param[in] ch     video channel for which AVC/H264 level is to set
 * @param[in] value  AVC/H264 level to set on the channel
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_avc_level(video_channel_t ch, uint32_t value);

/**
 * @brief This api is used to get AVC/H264 level on the video channel.
 * Applicable only for VID_FORMAT_H264_RAW, VID_FORMAT_H264_TS,
 * VID_FORMAT_H264_AAC_TS.
 * 
 * Typical Range: 10 to 52
 * 
 * @param[in]  ch     video channel for which AVC/H264 level is to get
 * @param[out] value  pointer to receive AVC/H264 level
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_avc_level(video_channel_t ch, uint32_t *value);

/**
 * @brief This api is used to enable/disable VUI NAL Unit in the H264
 * video channel.
 * 
 * Applicable only for VID_FORMAT_H264_RAW, VID_FORMAT_H264_TS,
 * VID_FORMAT_H264_AAC_TS.
 * 
 * Values: 0 – Disable, 1- Enable.
 * 
 * @param[in] ch     video channel for which VUI NAL Unit is to enable/disable
 * @param[in] value  flag to enable/disable VUI NAL Unit
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_vui(video_channel_t ch, uint32_t value);

/**
 * @brief This api is used to get the VUI NAL Unit status in the H264
 * video channel.
 * 
 * Applicable only for VID_FORMAT_H264_RAW, VID_FORMAT_H264_TS,
 * VID_FORMAT_H264_AAC_TS.
 * 
 * Values: 0 – Disable, 1- Enable.
 * 
 * @param[in]  ch     video channel for which VUI NAL Unit status is to get
 * @param[out] value  pointer to receive the VUI NAL Unit status
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_vui(video_channel_t ch, uint32_t *value);

/**
 * @brief This api is used to get picture timing NAL status in
 * the H264 video channel.
 * 
 * Applicable only for VID_FORMAT_H264_RAW, VID_FORMAT_H264_TS,
 * VID_FORMAT_H264_AAC_TS
 * 
 * @param[in] ch video channel for which picture timing NAL status is to get
 * @param[out] value pointer to receive picture timing NAL status
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_pict_timing(video_channel_t ch, uint32_t *value);

/**
 * @brief This api is used to enable/disable picture timing NAL in
 * the H264 video channel.
 * 
 * Applicable only for VID_FORMAT_H264_RAW, VID_FORMAT_H264_TS,
 * VID_FORMAT_H264_AAC_TS
 * 
 * Values: 0 – Disable, 1- Enable.
 * 
 * @param[in] ch video channel for which picture timing NAL status is to set
 * @param[in] value flag to enable/disable picture timing NAL
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_pict_timing(video_channel_t ch, uint32_t value);

/**
 * @brief This api is used to get the GOP Hierarchy level.
 * Applicable only for VID_FORMAT_H264_RAW,VID_FORMAT_H264_TS,
 * VID_FORMAT_H264_AAC_TS.
 * 
 * Typical Range: 0 to 4
 * 
 * @param[in] ch video channel of which GOP Hierarchy level is to get
 * @param[out] value pointer to recive GOP Hierarchy level
 * 
 * @return 0 on Success, -1 on Failure
 */
//int mxuvc_video_get_gop_hierarchy_level(video_channel_t ch, uint32_t *value);
int mxuvc_video_get_tsvc_level(video_channel_t ch, uint32_t *value);
#define mxuvc_video_get_gop_hierarchy_level     mxuvc_video_get_tsvc_level

/**
 * @brief This api is used to set the GOP Hierarchy level.
 * Applicable only for VID_FORMAT_H264_RAW,VID_FORMAT_H264_TS,
 * VID_FORMAT_H264_AAC_TS.
 * 
 * Typical Range: 0 to 4
 * 
 * @param[in] ch video channel for which GOP Hierarchy level is to set
 * @param[in] value GOP Hierarchy level
 * 
 * @return 0 on Success, -1 on Failure
 */
//int mxuvc_video_set_gop_hierarchy_level(video_channel_t ch, uint32_t value);
int mxuvc_video_set_tsvc_level(video_channel_t ch, uint32_t value);
#define mxuvc_video_set_gop_hierarchy_level     mxuvc_video_set_tsvc_level

/**
 * @brief This api is used to set the maximum size of the I frame in the
 * specified video channel. Applicable only for VID_FORMAT_H264_RAW,
 * VID_FORMAT_H264_TS, VID_FORMAT_H264_AAC_TS.
 * 
 * Typical Range: 0 to 64000.
 * 
 * @param[in] ch video channel for which max framesize is to set
 * @param[in] value maximum size of the I frame
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_max_framesize(video_channel_t ch, uint32_t value);

/**
 * @brief This api is used to get the maximum size of the I frame in the
 * specified video channel. Applicable only for VID_FORMAT_H264_RAW,
 * VID_FORMAT_H264_TS, VID_FORMAT_H264_AAC_TS.
 * 
 * Typical Range: 0 to 64000.
 * 
 * @param[in] ch video channel for which max framesize is to get
 * @param[out] value pointer to receive the maximum size of the I frame
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_max_framesize(video_channel_t ch, uint32_t *value);

int mxuvc_video_set_first_iframe_qp(video_channel_t ch, uint32_t value);
int mxuvc_video_get_first_iframe_qp(video_channel_t ch, uint32_t *value);

int mxuvc_video_set_tsvc_qpdelta (video_channel_t ch, int n, uint32_t value);
int mxuvc_video_get_tsvc_qpdelta(video_channel_t ch, int n, uint32_t *value);
#define mxuvc_video_set_tsvc_qpdelta0(ch, val)     mxuvc_video_set_tsvc_qpdelta(ch, 0, val)
#define mxuvc_video_get_tsvc_qpdelta0(ch, valptr)  mxuvc_video_get_tsvc_qpdelta(ch, 0, valptr)
#define mxuvc_video_set_tsvc_qpdelta1(ch, val)     mxuvc_video_set_tsvc_qpdelta(ch, 1, val)
#define mxuvc_video_get_tsvc_qpdelta1(ch, valptr)  mxuvc_video_get_tsvc_qpdelta(ch, 1, valptr)
#define mxuvc_video_set_tsvc_qpdelta2(ch, val)     mxuvc_video_set_tsvc_qpdelta(ch, 2, val)
#define mxuvc_video_get_tsvc_qpdelta2(ch, valptr)  mxuvc_video_get_tsvc_qpdelta(ch, 2, valptr)
#define mxuvc_video_set_tsvc_qpdelta3(ch, val)     mxuvc_video_set_tsvc_qpdelta(ch, 3, val)
#define mxuvc_video_get_tsvc_qpdelta3(ch, valptr)  mxuvc_video_get_tsvc_qpdelta(ch, 3, valptr)
#define mxuvc_video_set_tsvc_qpdelta4(ch, val)     mxuvc_video_set_tsvc_qpdelta(ch, 4, val)
#define mxuvc_video_get_tsvc_qpdelta4(ch, valptr)  mxuvc_video_get_tsvc_qpdelta(ch, 4, valptr)
#define mxuvc_video_set_tsvc_qpdelta5(ch, val)     mxuvc_video_set_tsvc_qpdelta(ch, 5, val)
#define mxuvc_video_get_tsvc_qpdelta5(ch, valptr)  mxuvc_video_get_tsvc_qpdelta(ch, 5, valptr)
#define mxuvc_video_set_tsvc_qpdelta6(ch, val)     mxuvc_video_set_tsvc_qpdelta(ch, 6, val)
#define mxuvc_video_get_tsvc_qpdelta6(ch, valptr)  mxuvc_video_get_tsvc_qpdelta(ch, 6, valptr)
#define mxuvc_video_set_tsvc_qpdelta7(ch, val)     mxuvc_video_set_tsvc_qpdelta(ch, 7, val)
#define mxuvc_video_get_tsvc_qpdelta7(ch, valptr)  mxuvc_video_get_tsvc_qpdelta(ch, 7, valptr)


/**@}*/

/** @defgroup dewarp_control Dewarp Parameters (Set/Get)
  * @{
  */

/** List of valid dewarp modes. Each dewarp mode has its own data structures as variable,
 associated to each modes are different. Refer mxuvc_dewarp.h to see more details on
 individual parameters.

Note: Some modes does not have any parameters to be sent to the firmware and as a result they do not
have any structures associated with them.
 */
typedef enum {
    /** non-dewarp mode */
    EMODE_OFF = 0,
    /** ZCL mode */
    EMODE_WM_ZCL,
    /** ZCL cylindrical mode */
    EMODE_WM_ZCLCylinder,
    /** ZCL stretch mode */ 
    EMODE_WM_ZCLStretch,
    /** 1 panel eptz mode */
    EMODE_WM_1PanelEPTZ,
    /** 1 panel sweep mode */
    EMODE_WM_Sweep_1PanelEPTZ,
    /** magnify mode */
    EMODE_WM_Magnify,
    /** 1 circle panorama view pan mode */
    EMODE_TM_1CircPanoViewPan,
    /** panorama view pan mode */
    EMODE_CM_CircPanoViewPan,
    /** 1 panel eptz mode for ROI */
    EMODE_WM_ROI_1PanelEPTZ,
    /** custom mode for 360 view camera mode */
    EMODE_CUSTOM_CAM360,
    /** 360 circle panorama view mode */
    EMODE_TM_360CircPanoView_1PanelNonPerspPan,
    EMODE_MAX,
} dewarp_mode_t;

/** structure containing dewarp configuration parameters */
typedef struct
{
    /** Compositor status for specific channel. Read Only */
    int composite;
    /** Dewarp Status/ Number for specific channel. Read Only */
    int dewarp;
    /** Number of panels for specific channel. Read Only */
    int panels;
    /** Sensor used */
    int sensor;
    /** Default lens field of view. Read Only */
    int fov;
    /** Default radius. Read Only */
    int radius;
    /** Default hshift. Read Only */
    int hshift;
    /** Default lens vshift. Read Only */
    int vshift;

} config_params_t;

/**
 * @brief This api is used to set dewarp paramterers used for the given video channel.
 * 
 * @param[in]  ch       video channel on which the parameters are set
 * @param[in]  panel    panel number, applicable only for multi panel modes
 * @param[in]  mode     type of dewarp mode as defined in the enum
 * @param[out] params   mode specific dewarp parameters
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_dewarp_params(video_channel_t ch, int panel, dewarp_mode_t mode, dewarp_params_t* params);

/**
 * @brief This api is used to get dewarp paramterers used for the given video channel.
 * 
 * @param[in]  ch       video channel on which the parameters are required
 * @param[in]  panel    panel number, applicable only for multi panel modes
 * @param[in]  mode     type of dewarp mode as defined in the enum
 * @param[out] params   mode specific dewarp parameters
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_dewarp_params(video_channel_t ch, int panel, dewarp_mode_t* mode, dewarp_params_t* params);

/**
 * @brief This API is used to query the current dewarp settings of the given
 * video channel.
 * 
 * @param[in] ch [description]
 * @param[out] params pointer to struture @ref config_params_t containing dewarp
 *                    settings updated by MXUVC
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_config_params(video_channel_t ch, config_params_t* params);

/**@}*/

/** @defgroup compositor_control Compositor Parameters (Set/Get)
  * @{
  */

/** enum containing list of valid compositor modes */
typedef enum {
    /** OFF mode */
    PMODE_OFF = 0,
    /** ON mode */
    PMODE_ON,
    /** select mode */
    PMODE_SELECT,
    /** deselect mode */
    PMODE_UNSELECT,
    /** pause demo mode */
    PMODE_PAUSE_DEMO,
    /** resume demo mode */
    PMODE_UNPAUSE_DEMO,
    PMODE_MAX,
} panel_mode_t;

/** structure containing panel specific information */
typedef struct
{
    /** x offset */
    int x;
    /** y offset */
    int y;
    /** width */
    int width;
    /** height */
    int height;
} panel_params_t;

/**
 * @brief This api is used to get compositor parameters used for the given video channel.
 * 
 * @param[in] ch video channel on which the parameters are required
 * @param[in] panel panel number, applicable only for multi panel modes
 * @param[in] mode type of compositor mode as defined in the enum
 * @param[out] params mode specific compositor parameters

 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_compositor_params(video_channel_t ch, int panel, panel_mode_t* mode, panel_params_t* params);

/**
 * @brief This api is used to set compositor parameters for given video channel.
 * 
 * @param[in] ch video channel on which the parameters are required to set
 * @param[in] panel panel number, applicable only for multi panel modes
 * @param[in] mode type of compositor mode as defined in the enum
 * @param[in] params mode specific compositor parameters

 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_compositor_params(video_channel_t ch, int panel, panel_mode_t mode, panel_params_t* params);

/**@}*/

/** @defgroup sensor_control Sensor Parameters (Set/Get)
  * @{
  */

/**
 * @brief This api is used to set vertical flip of the given video channel.
 * 
 * Values: 1 Enable, 0 Disable
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_flip_vertical(video_channel_t ch, video_flip_t value);

/**
 * @brief This api is used to get vertical flip status of the given video channel.
 * 
 * Values: 1 Enable, 0 Disable
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter value updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_flip_vertical(video_channel_t ch, video_flip_t *value);

/**
 * @brief This api is used to set horizontal flip of the given video channel.
 * 
 * Values: 1 Enable, 0 Disable
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_flip_horizontal(video_channel_t ch, video_flip_t value);

/**
 * @brief This api is used to get horizontal flip status of the given video channel.
 * 
 * Values: 1 Enable, 0 Disable
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter value updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_flip_horizontal(video_channel_t ch, video_flip_t *value);

/**
 * @brief This api is used to set image contrast of the given video channel.
 * 
 * Typical Range: 0 to 200
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_contrast(video_channel_t ch, uint16_t value);

/**
 * @brief This api is used to get image contrast of the given video channel.
 * 
 * Typical Range: 0 to 200
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter value updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_contrast(video_channel_t ch, uint16_t *value);

/**
 * @brief This api is used to set image zoom step of the given video channel.
 * 
 * Typical Range: 0 to 100
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_zoom(video_channel_t ch, uint16_t value);

/**
 * @brief This api is used to get image zoom step of the given video channel.
 * 
 * Typical Range: 0 to 100
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter value updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_zoom(video_channel_t ch, uint16_t *value);

/**
 * @brief This api is used to set image pan level of the given video channel.
 * 
 * Typical Range: -648000 to 648000
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_pan(video_channel_t ch, int32_t value);

/**
 * @brief This api is used to get image pan level of the given video channel.
 * 
 * Typical Range: -648000 to 648000
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter value updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_pan(video_channel_t ch, int32_t *value);

/**
 * @brief This api is used to set image tilt level of the given video channel.
 * 
 * Typical Range: -648000 to 648000
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] tilt value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_tilt(video_channel_t ch, int32_t tilt);

/**
 * @brief This api is used to get image tilt level of the given video channel.
 * 
 * Typical Range: -648000 to 648000
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] tilt pointer to parameter value updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_tilt(video_channel_t ch,int32_t *tilt);

/**
 * @brief This api is used to set image pan and tilt level of the given video channel.
 * 
 * Typical Range: PAN: -648000 to 648000;  TILT -648000 to 648000
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] pan pan value to be set in the camera
 * @param[in] tilt tilt value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_pantilt(video_channel_t ch, int32_t pan, int32_t tilt);

/**
 * @brief This api is used to get image pan and tilt level of the given video channel.
 * 
 * Typical Range: PAN: -648000 to 648000;  TILT -648000 to 648000
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] pan pointer to pan value updated by the camera
 * @param[out] tilt pointer to tilt value updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_pantilt(video_channel_t ch, int32_t *pan, int32_t *tilt);

/**
 * @brief This api is used to set image brightness of the given video channel.
 * 
 * Typical Range: -255 to 255
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_brightness(video_channel_t ch, int16_t value);

/**
 * @brief This api is used to get image brightness of the given video channel.
 * 
 * Typical Range: -255 to 255
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_brightness(video_channel_t ch, int16_t *value);

/**
 * @brief This api is used to set image hue of the given video channel.
 * 
 * Typical Range: -18000 to 18000
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_hue(video_channel_t ch, int16_t value);

/**
 * @brief This api is used to get image hue of the given video channel.
 * 
 * Typical Range: -18000 to 18000
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_hue(video_channel_t ch, int16_t *value);

/**
 * @brief This api is used to set image gamma of the given video channel.
 * 
 * Typical Range: 100 to 300
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_gamma(video_channel_t ch, uint16_t value);

/**
 * @brief This api is used to get image gamma of the given video channel.
 * 
 * Typical Range: 100 to 300
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_gamma(video_channel_t ch, uint16_t *value);

/**
 * @brief This api is used to set image saturation of the given video channel.
 * 
 * Typical Range: 0 to 200
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_saturation(video_channel_t ch, uint16_t value);

/**
 * @brief This api is used to get image saturation of the given video channel.
 * 
 * Typical Range: 0 to 200
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_saturation(video_channel_t ch, uint16_t *value);

/**
 * @brief This api is used to set image gain of the given video channel.
 * 
 * Typical Range: 1 to 100
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_gain(video_channel_t ch, uint16_t value);

/**
 * @brief This api is used to get image gain of the given video channel.
 * 
 * Typical Range: 1 to 100
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_gain(video_channel_t ch, uint16_t *value);

/**
 * @brief This api is used to set image sharpness of the given video channel.
 * 
 * Typical Range: 1 to 100
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_sharpness(video_channel_t ch, uint16_t value);

/**
 * @brief This api is used to get image sharpness of the given video channel.
 * 
 * Typical Range: 1 to 100
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_sharpness(video_channel_t ch, uint16_t *value);

/**
 * @brief This api is used to set Controls the maximum sensor analog gain in
 * auto exposure algorithm.
 * 
 * Typical Range: 0 to 15
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_max_analog_gain(video_channel_t ch, uint32_t value);

/**
 * @brief This api is used to get the maximum sensor analog gain in
 * auto exposure algorithm.
 * 
 * Typical Range: 0 to 15
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_max_analog_gain(video_channel_t ch, uint32_t *value);

/**
 * @brief This api is used to enable/disable histogram equalization,
 * which creates more contrast to the image.
 * 
 * Typical Range: 0 to disable, 1 to enable
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_histogram_eq(video_channel_t ch, histo_eq_t value);

/**
 * @brief This api is used to get current status of histogram equalization.
 * 
 * Typical Range: 0 to disable, 1 to enable
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_histogram_eq(video_channel_t ch, histo_eq_t *value);

/**
 * @brief This api is used to set different strengths of the sharpening
 * filter.
 * 
 * Typical Range: 0 to 2, 2 being the strongest 
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_sharpen_filter(video_channel_t ch, uint32_t value);

/**
 * @brief This api is used to get current strengths of the sharpening
 * filter.
 * 
 * Typical Range: 0 to 2, 2 being the strongest 
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_sharpen_filter(video_channel_t ch, uint32_t *value);

/**
 * @brief This api is used to set minimun framerate that the auto exposure
 * algorithm can go to.
 * 
 * Typical Range: 0 to 30 
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_min_exp_framerate(video_channel_t ch, uint32_t value);

/**
 * @brief This api is used to get minimum exposure framerate.
 * 
 * Typical Range: 0 to 30
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_min_exp_framerate(video_channel_t ch, uint32_t *value);

/**
 * @brief This api is used to set temporal filter strength.
 * Value 0 disables the temporal filter. Value 1 to 7 sets the filter strength.
 * 
 * Typical Range: 0 to 7 
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_tf_strength(video_channel_t ch, uint32_t value);

/**
 * @brief This api is used to get temporal filter strength.
 * 
 * Typical Range: 0 to 7
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_tf_strength(video_channel_t ch, uint32_t *value);

/**
 * @brief This api is used to controls the auto exposure algorithm to adjust
 * the sensor analog gain and exposure based on different lighting conditions.
 * 
 * Typical Range: 0 to 256 
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value value to be set in the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_gain_multiplier(video_channel_t ch, uint32_t value);

/**
 * @brief This api is used to get gain multiplier that was set by @ref
 * mxuvc_video_set_gain_multiplier
 * 
 * Typical Range: 0 to 256
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] value pointer to parameter updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_gain_multiplier(video_channel_t ch, uint32_t *value);

/**
 * @brief This api is used to set the sensor exposure. 
 * Exposure is the sensor integration time in number of lines. 
 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value exposure value - Range 1 to 65535 
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_sensor_exposure(video_channel_t ch, uint16_t value);

/**
 * @brief This api is used to set the noise filter mode (auto or manual)
 * and the noise filter strength in the manual mode.
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] sel selects to noise filter mode (Auto or Manual)
 * @param[in] value noise filter strength in the Manual mode - Range 0 to 100
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_nf(video_channel_t ch, noise_filter_mode_t sel, uint16_t value);

/**
 * @brief This api is used to get the noise filter mode (auto or manual)
 * and the noise filter strength in the manual mode.
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] sel selects to noise filter mode (Auto or Manual)
 * @param[out] value pointer to parameter updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_nf(video_channel_t ch, noise_filter_mode_t *sel, uint16_t *value);

/**
 * @brief This api is used to preload  the auto white balance (AWB) gains.
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] params structure containing the AWB gains
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_wb(video_channel_t ch, awb_params_t* params);

/**
 * @brief This api is used to set the camera wide dynamic range mode (WDR) (auto or manual)
 * and the WDR control intensity in the manual mode.
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] mode selects to WDR mode (Auto or Manual)
 * @param[in] value WDR control intensity value in the Manual mode - Range 0 to 255
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_wdr(video_channel_t ch, wdr_mode_t mode, uint8_t value);

/**
 * @brief This api is used to get the camera wide dynamic range mode (WDR) (auto or manual)
 * and the WDR control intensity in the manual mode.
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[out] mode selects to WDR mode (Auto or Manual)
 * @param[out] value pointer to parameter updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_wdr(video_channel_t ch, wdr_mode_t *mode, uint8_t *value);

/**
 * @brief This api is used to set sensor gain.
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] value sensor gain value
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_sensor_gain(video_channel_t ch, unsigned int value);

/**
 * @brief This api is used to enable/disable zonal white balance and set the white
 * balance zone.
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] sel enable/disable zonal white balance
 * @param[in] value white balnce zone value - Range 0 to 63
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_zone_wb(video_channel_t ch, zone_wb_set_t sel, uint16_t value);

/**
 * @brief This api is used to get the status of zonal white balance and the white balance
 * zone.
 *
 * @param[in] ch video channel on which the parameter is set
 * @param[out] sel status of zonal white balance
 * @param[out] value pointer to parameter updated by the camera
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_zone_wb(video_channel_t ch, zone_wb_set_t *sel, uint16_t *value);

/**
 * @brief This API is used to select the power line frequency of the operating region.
 * Sensor exposure value under the auto-exposure algorithm will be adjusted to avoid
 * flickering caused by power level oscillation. 
 * 0 disables this function, and the values of 1 and 2 represents 50 and 60Hz power
 * line frequency respectively.
 * 
 * @param[in] ch video channel on which the parameter is set
 * @param[in] mode value indicating power line frequency mode
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_pwr_line_freq(video_channel_t ch,pwr_line_freq_mode_t mode);

/**
 * @brief This api is used to get the status of power line frequency of the operating
 * region.
 *
 * @param[in] ch video channel on which the parameter is set
 * @param[out] mode pointer to parameter updated by the camera
 * 0 – disable, 1 – 50Hz, 2 – 60Hz
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_pwr_line_freq(video_channel_t ch,pwr_line_freq_mode_t *mode);

/* Get supported resolutions for the current video format */
/* Run until return value > 0 */
//int mxuvc_get_supported_resolutions(video_channel_t ch, uint16_t *width,
//                                          uint16_t *height);
int mxuvc_get_camera_mode(camer_mode_t *mode);

/**
 * @brief NOT SUPPORTED
 * 
 * @param ch TBD
 * @param fmt TBD
 * 
 * @return TBD
 */
int mxuvc_video_set_format(video_channel_t ch, video_format_t fmt);

/**
 * @brief NOT SUPPORTED
 * 
 * @param ch TBD
 * @param mode TBD
 * 
 * @return TBD
 */
int mxuvc_video_set_brightness_mode(video_channel_t ch, brightness_mode_t mode);

/**
 * @brief NOT SUPPORTED
 * 
 * @param ch TBD
 * @param mode TBD
 * 
 * @return TBD
 */
int mxuvc_video_get_brightness_mode(video_channel_t ch, brightness_mode_t *mode);

/**
 * @brief NOT SUPPORTED
 * 
 * @param ch TBD
 * @param mode TBD
 * 
 * @return TBD
 */
int mxuvc_video_set_contrast_mode(video_channel_t ch, contrast_mode_t mode);

/**
 * @brief NOT SUPPORTED
 * 
 * @param ch TBD
 * @param mode TBD
 * 
 * @return TBD
 */
int mxuvc_video_get_contrast_mode(video_channel_t ch, contrast_mode_t *mode);

/**
 * @brief NOT SUPPORTED
 * 
 * @param ch TBD
 * @param mode TBD
 * 
 * @return TBD
 */
int mxuvc_video_set_saturation_mode(video_channel_t ch, saturation_mode_t mode);

/**
 * @brief NOT SUPPORTED
 * 
 * @param ch TBD
 * @param mode TBD
 * 
 * @return TBD
 */
int mxuvc_video_get_saturation_mode(video_channel_t ch, saturation_mode_t *mode);

/**
 * @brief NOT SUPPORTED
 * 
 * @param ch TBD
 * @param info TBD
 * 
 * @return TBD
 */
int mxuvc_video_set_sinter(video_channel_t ch, sinter_info_t *info);

/**
 * @brief NOT SUPPORTED
 * 
 * @param ch TBD
 * @param info TBD
 * 
 * @return TBD
 */
int mxuvc_video_get_sinter(video_channel_t ch, sinter_info_t *info);

/**
 * @brief NOT SUPPORTED
 * 
 * @param ch TBD
 * @param params TBD
 * 
 * @return TBD
 */
int mxuvc_video_set_vbr_params(video_channel_t ch, vbr_params_t *params);

/**
 * @brief NOT SUPPORTED
 * 
 * @param ch TBD
 * @param params TBD
 * 
 * @return TBD
 */
int mxuvc_video_get_vbr_params(video_channel_t ch, vbr_params_t *params);

/**
 * @brief This api is used to get the current sensor framerate.
 *
 * @param[in] ch video channel on which the parameter is set.
 * @param[out] framerate pointer to parameter updated by the camera.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_sensor_framerate(video_channel_t ch, uint32_t *framerate);

/**
 * @brief This api is used to set the sensor framerate.
 *
 * @param[in] ch video channel on which the parameter is set.
 * @param[in] framerate framerate to set on the channel.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_sensor_framerate(video_channel_t ch, uint32_t framerate);

/**
 * @brief This API is used to set the isp ae roi crop paramters on the specified channel.
 * 
 * @param[in] ch    video channel on which the crop information is to set
 * @param[in] info  ae roi crop parameters to set on the channel
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_set_isp_roi(video_channel_t ch, isp_ae_roi_info_t *info);

/**
 * @brief This API is used to get the isp AE paramters of the specified channel.
 * 
 * @param[in]  ch    video channel on which the crop information is to be queried
 * @param[out] info  pointer to receive the isp AE parameters
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_video_get_isp_roi(video_channel_t ch, isp_ae_roi_info_t *info);

/**@}*/

/**@}*/

/** @defgroup audio_system Audio Subsystem
  * @{
  */

/** enum to indicate the format type supported */
typedef enum {
    /** PCM format */
    AUD_FORMAT_PCM_RAW  = 0,
    /** AAC format */
    AUD_FORMAT_AAC_RAW  = 1,
    /** OPUS format */
    AUD_FORMAT_OPUS_RAW = 2,
    /** number of audio formats supported */
    NUM_AUD_FORMAT
} audio_format_t;

/** enum to indicate the MXUVC audio channel type*/
typedef enum {
    /** channel for capturing audio in PCM format */
    AUD_CH1 = 0,
    /** channel for capturing audio in encoded format (AAC or OPUS) */
    AUD_CH2,
    /** number of audio channels available */
    NUM_AUDIO_CHANNELS
} audio_channel_t;

/** structure containing the parameters of audio data received in
 callback associated to audio channel */
typedef struct
{
    /** audio timestamp in terms of ticks of 90khz clock where
    each tick corresponds to 1/(90 * 1000) sec or 1/90 ms */
    long long timestamp;
    /** size of the audio frame received */
    int framesize;
    /** sampling frequency at which the audio frame is captured */
    int samplefreq;
    /** number of audio channels captured by the microphone and/or
    encoded */
    int channelno;
    /** audio object type with which the Audio stream is encoded.
     This is useful to construct the ADTS Header in case of AAC
     encoded stream. Ignore in case of PCM or other format. */
    int audioobjtype;
    /** format of the captured audio */
    audio_format_t format;
    /** pointer to audio frame data */
    unsigned char *dataptr;
} audio_params_t;

/**
 * @brief audio callback function to register with mxuvc audio for receiving
 * audio capture data.
 * 
 * @param buffer pointer to the audio data received from the camera
 * @param size size of the audio data received from the camera
 * @param format format of the audio frame received
 * @param ts audio timestamp in terms of ticks of 90khz clock where
 *           each tick corresponds to 1/(90 * 1000) sec or 1/90 ms
 * @param user_data pointer to user context/user data that the user set
 *                  when registering the callback function.
 *                  The pointer value is returned as given to MXUVC at the
 *                  time of registering the callback function i.e.
 *                  in @ref mxuvc_audio_register_cb
 * @param param pointer to the audio parameters in the current frame
 *              received from the audio channel in the callback function
 *              
 */
typedef void (*mxuvc_audio_cb_t)(unsigned char *buffer, unsigned int size,
                                    audio_format_t format, uint64_t ts, void *user_data, audio_params_t *param);

/** @defgroup audio_init_deinit Init / Deinit
  * @{
  */

/**
 * @brief This API is used to initialize the Linux audio interface on the host system.
 * 
 * @param[in] backend string representing the audio backend. It is "alsa" in our case.
 * @param[in] options semi-colon separated list of options. Following options can be
 *                    specified with this:<br><br>
 *                    <b>device:</b>This is a mandatory parameter used to specify the ALSA
 *                    name of the GEO Camera, typically “Condor”.<br><br>
 *                    <b>audio_sampling_rate:</b> This parameter is used to specify the
 *                    sampling rate at which the audio needs to be captured.
 *                    It is not a mandatory parameter. Default value is 24khz.
 * 
 * @return 0 on Success, -1 on Failure
 * 
 * Example Usage:
 * @code
 * mxuvc_audio_init("alsa", "device = Condor; audio_sampling_rate = 24000");
 * @endcode
 * 
 * or
 * 
 * @code
 * mxuvc_audio_init("alsa", "device = Condor");
 * @endcode
 */
int mxuvc_audio_init(const char *backend, const char *options);

/**
 * @brief This API is used to free all the memory allocated by the MXUVC.
 * This function automatically calls @ref mxuvc_audio_stop if the audio has
 * not been stopped yet, it is therefore not necessary to explicitly call
 * @ref mxuvc_audio_stop before calling @ref mxuvc_audio_deinit.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_audio_deinit();

/**@}*/


/** @defgroup audio_register Registration Of Audio Callback
  * @{
  */

/**
 * @brief This API is used to register the call back function with the MXUVC.
 * MXUVC calls this registered callback function when the audio data is available
 * from the camera on the specified channel. Application can use this function
 * to process the audio data received from the camera.
 * 
 * @param[in] ch audio channel on which the call back function is registered
 * @param[in] func pointer to the call back function to be registered with mxuvc
 * @param[in] user_data pointer to user context/ user data that the user wants
 *            to access inside the callback function.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_audio_register_cb(audio_channel_t ch, mxuvc_audio_cb_t func, void *user_data);

/**@}*/

/** @defgroup audio_start_stop Start/Stop Audio Capture
  * @{
  */

/**
 * @brief This API is used to start the capture of audio data from the
 * specified audio channel in the camera.
 * 
 * @param[in] ch audio channel on which the audio capture needs to be started
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_audio_start(audio_channel_t ch);

/**
 * @brief This API is used to stop the capture of audio data from a specified
 * audio channel in the camera.
 * 
 * @param[in] ch audio channel on which the audio capture needs to be started
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_audio_stop(audio_channel_t ch);

/**
 * @brief This API is used to check if the camera is active. It can be
 * periodically called to check the availability of the camera.
 * This function returns an error as not alive in the following conditions:
 * • the camera has been unplugged
 * • the camera or the USB host is no longer responding
 * 
 * @return 0 if Not alive, 1 if alive
 */
int mxuvc_audio_alive();

/**@}*/

/** @defgroup audio_control Audio Parameters (Set/Get)
  * @{
  */

/**
 * @brief This API is used to set the audio capture sampling rate.
 * This parameter once set will be affecting both the audio channels AUD_CH1 and 
 * AUD_CH2. We cannot set different sampling rate separately for each audio channel.
 * 
 * @param[in] ch audio channel for which the sampling rate is to be set
 * @param samplingFr ampling frequency to be set default range 8000 – 24000
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_audio_set_samplerate(audio_channel_t ch, int samplingFr);

/**
 * @brief This API is used to get the sampling rate of the audio capture.
 * 
 * @param[in] ch audio channel for which the sampling rate is to be queried
 * 
 * @return the current sampling rate on Success, -1 on Failure
 */
int mxuvc_audio_get_samplerate(audio_channel_t ch);

/**
 * @brief This API is used to set the microphone gain.
 * This will work only when AGC is off.
 * 
 * @param[in] volume microphone gain level. Range is from 0 to 100 without any units.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_audio_set_volume(int volume);

/**
 * @brief This API is used to get the microphone gain.
 * 
 * @return microphone gain level
 */
int mxuvc_audio_get_volume();

/**
 * @brief This API is used to mute or unmute the micro phone.
 * It will mute or unmute all the audio input channels.
 * 
 * @param[in] bMute microphone mute value. 0 - unmute, 1 - mute
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_audio_set_mic_mute(int bMute);  /* mute=1, unmute=0 */

/**
 * @brief This API is used to mute or unmute only the left microphone
 * in case of stereo. This will work only when ASP is off (in JSON).
 * 
 * @param[in] bMute microphone mute value. 0 - unmute, 1 - mute
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_audio_set_left_mic_mute(int bMute); /* mute=1, unmute=0 */

/**
 * @brief This API is used to mute or unmute only the right microphone
 * in case of stereo. This will work only when ASP is off (in JSON).
 * 
 * @param[in] bMute microphone mute value. 0 - unmute, 1 - mute
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_audio_set_right_mic_mute(int bMute);

/**
 * @brief This API is used to get the bitrate of the compressed audio 
 * Applicable only for AUD_CH2.
 * 
 * @param[out] value audio bitrate
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_audio_get_bitrate(uint32_t *value);

/**
 * @brief This API is used to set the bitrate of the compressed audio 
 * Applicable only for AUD_CH2.
 * 
 * @param[in] value bitrate to be set. (Range 16000 to 64000)
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_audio_set_bitrate(uint32_t value);

/**@}*/

/**@}*/

/** @defgroup alert_system Alert Subsystem
  * @{
  */

/** enum for stating the current state of audio alarm */
typedef enum {
    /** audio alert started */
    AUDIOALERT_STARTED = 1,
    /** audio alert stopped */
    AUDIOALERT_STOPPED,
}audio_alert_state;

/** structure contains the information of audio alert that was generated
 by the GEO camera */
typedef struct {
    /** audio intensity present in environment when audio alert was generated */
    uint32_t audioThresholdDB;
    /** indicates the state of the audio alert event at the time of the event
     generation. AUDIOALERT_STARTED indicates that the audio intensity has just
     started going above the user specified threshold limit. AUDIOALERT_STOPPED
     indicates that the audio intensity has gone below the user specified
     threshold limit. */
    audio_alert_state state;
} audio_alert_info ;

/**
 * @brief Callback function to register with MXUVC for getting alert notification.
 * 
 * @param[out] audalert_info pointer to audio alert structure @ref audio_alert_info
 *                           which holds the details of the alert event generated
 *                           by the firmware
 *                           
 * @param[out] user_data pointer to user context/ user data that the user wants to
 *                       access inside the callback function. The pointer value is
 *                       returned as given to MXUVC at the time of registering the
 *                       callback function.
 * 
 */
typedef void (*mxuvc_audio_alert_cb_t)(audio_alert_info * audalert_info, void *user_data);

/** @defgroup alert_init_deinit Init / Deinit
  * @{
  */

/**
 * @brief This API is used to initialize the Alert subsystem.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_alert_init(void);

/**
 * @brief This API is used to release all the resources allocated by the
 * alert subsystem.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_alert_deinit(void);

/**@}*/

/** @defgroup alert_register Registration Of Alert Callback
  * @{
  */

/**
 * @brief This API is used to register the callback function to the audio alert module.
 * 
 * @param[in] func pointer to the callback function which is called when alert is
 *                 generated.
 * @param[in] user_data pointer to user context/user data that the user wants to access
 *                      inside the callback function.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_alert_audio_register_cb(mxuvc_audio_alert_cb_t func,void *user_data);

/**@}*/

/** @defgroup alert_control Alert Parameters (Set/Get)
  * @{
  */

/**
 * @brief This API is used to enable the audio alert module in firmware.
 * Registred callback function will be called as and when alert occurs.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_alert_audio_enable(void);

/**
 * @brief This API is used to set audio intensity threshold in terms of decibels,
 * above which the audio alert should be triggered.
 * 
 * @param[in] audioThresholdDB audio intensity threshold above which the audio alert
 *                             will be generated
 *                             
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_alert_audio_set_threshold(unsigned int audioThresholdDB);

/**
 * @brief This API is used is used to disable audio alert.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_alert_audio_disable(void);

/**
 * @brief This API is used to get the current intensity of the audio in the
 * monitoring environment, in terms of decibels, perceived by the microphone.
 * 
 * @param[out] audioIntensityDB information passed by the camera indicating current
 *                         intensity of the audio in terms decibels, in the
 *                         monitoring environment as perceived by the microphone.
 *                         
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_get_current_audio_intensity(unsigned int *audioIntensityDB);

/**@}*/

/** @defgroup i2c_notification_system I2C Notification Subsytem
  * @{
  */

/** @defgroup i2c_notification_init I2C Notification Subsytem Init/Deinit
  * @{
  */
/**
 * @brief This API is used to detect an i2c device and on success 
 * return a positive handle.
 * 
 * @param[in] i2c_instance i2c device instance (0 or 1).
 * @param[in] i2c_device_type i2c device type 
 *            (0: std 8b | 1: std 16b | 2: sccb 8b | 3: sccb 16b | 4: no subaddr)
 * @param[in] i2c_device_addr i2c device slave address.
 * @param[in] chip_id_reg Normally i2c slave device has a chip id register with a default value, which is
 *	                  typically used to detect the chip and its version.
 *			  If chip_id_reg and chip_id_reg_value both are provided as zero then it will be
 *			  treated as no i2c device detection is required.
 * @param[in] chip_id_reg_value default value of chip id register..
 *
 * @return +ve handle on Success, -1 on Failure.
 */
int mxuvc_i2c_open(int i2c_instance, 
		   int i2c_device_type, 
		   uint16_t i2c_device_addr, 
		   uint16_t chip_id_reg, 
		   uint16_t chip_id_reg_value);
/** 
 * @brief This API is used to close an opened i2c notification interface.
 *
 * @param[in] handle handle of an opened instance (handle return value from mxuvc_i2c_open()).
 *
 * @return 0 on Success, -1 on Failure.
 */
int mxuvc_i2c_close(int handle);

/**@}*/

/** @defgroup i2c_notification_register Registration Of I2C Notification Callback
  * @{
  */
/**
 * @brief This API gets called when i2c notification happens.
 * 
 * @param[out] value i2c register value when i2c notification happened.
 *
 * @return void.
 */

typedef void (*i2c_callback_fn_t)(int value);
/**
 * @brief This API is used to register a callback function and specify 
 * 	  few other informations reagrding i2c read characteristics.
 * 
 * @param[in] handle handle of an opened instance (handle return value from mxuvc_i2c_open()).
 * @param[in] i2c_callback this function returns the i2c register value when the threshold is crossed.
 * @param[in] polling interval in ms unit, polling interval step unit is in 500ms. 
 * 	      Example: 500 ms, 1000 ms or 1500 ms.
 * @param[in] reg_addr i2c device register address.
 * @param[in] read_len i2c read data length in bytes(1 or 2) in bits [6:0],
 *                     endianness in bit [7] if reading 2 bytes (0=little endian, 1=big endian)
 * @return 0 on Success, -1 on Failure.
 */
int mxuvc_i2c_register_notification_callback(int handle, 
			i2c_callback_fn_t i2c_callback, 
			uint32_t polling_interval_ms, 
			uint16_t reg_addr,
			uint32_t read_len);
/**@}*/

/** @defgroup i2c_notification_getset I2C Notification Parameters (Set/Get)
  * @{
  */

/**
 * @brief This API is used to write an i2c register.
 * 
 * @param[in] handle handle of an opened instance (handle return value from mxuvc_i2c_open()). 
 * @param[in] reg_addr i2c device register address.
 * @param[in] value value to be written.
 * @param[in] write_len 2c data length in bytes(1 or 2).
 *
 * @return 0 on Success, -1 on Failure.
 */
int mxuvc_i2c_notification_write_register(int handle, 
			uint16_t reg_addr, uint16_t value,
			uint32_t write_len);
/**
 * @brief This API is used to write an i2c register.
 * 
 * @param[in] handle handle of an opened instance (handle return value from mxuvc_i2c_open()).
 * @param[in] reg_addr reg_addr i2c device register address.
 * @param[out] value value to be read.
 * @param[in] read_len 2c data length in bytes(1 or 2).
 *
 * @return 0 on Success, -1 on Failure.
 */
int mxuvc_i2c_notification_read_register(int handle, 
			uint16_t reg_addr, uint16_t *value,
			uint32_t read_len);
/**
 * @brief This API is used to add a lower and upper threshold to compare the read i2c value. Threshold value pairs
 *	  added must be in an incremental order and without any overlap. Also at any point of time maximum five
 *        instances of threshold can be added. When read i2c value sees a state change then registered callback
 *        function i2c_callback() gets invoked, here state change means when a value is below lower threshold or
 *        when it just crosses higher threshold value of a threshold pair and vice versa.
 * 
 * @param[in] handle handle of an opened instance (handle return value from mxuvc_i2c_open()).
 * @param[in] low_threshold lower threshold value.
 * @param[in] high_threshold igher threshold value.
 *
 * @return 0 on Success, -1 on Failure.
 */
int mxuvc_i2c_notification_add_threshold(int handle, 
			uint16_t low_threshold, 
			uint16_t high_threshold);
/** 
 * @brief This API is used to remove an already added lower and upper threshold value.
 *
 * @param[in] handle handle of an opened instance (handle return value from mxuvc_i2c_open()).
 * @param[in] low_threshold lower threshold value.
 * @param[in] high_threshold igher threshold value.
 *
 * @return 0 on Success, -1 on Failure.
 */
int mxuvc_i2c_notification_remove_threshold(int handle, 
			uint16_t low_threshold, 
			uint16_t high_threshold);
/**@}*/

/**@}*/

/** @defgroup mxuvc_gpio_register_notification GPIO Notification Subsystem
  * @{
  */
/**
 * @brief This API gets called when gpio notification happens.
 * 
 * @param[out] gpio_num the set bit locations in this 32bit value will specify which gpio value is changed .
 * @param[out] gpio_value the bit location value w.r.t the gpio_num set bit value will specify the new value of gpio.
 * @return void.
 */
typedef void (*gpio_callback_fn_t)(uint32_t gpio_num, uint32_t gpio_value);
/**
 * @brief This API is used to register a callback function, gpio number 
 * 	  and polling interval for receiving a gpio state change notification.
 * 
 * @param[in] gpio_callback function returns gpio_num & gpio_value to notify the state change of gpio.
 * @param[in] polling interval in ms unit, lowest supported polling interval is 10ms. 
 * @param[in] gpio_number the set bit locations in this 32bit value will specify which gpio/s value change 
 *        is expected to be notified.
 *
 * @return 0 on Success, -1 on Failure.
 */
int mxuvc_gpio_notification_start(
		    gpio_callback_fn_t gpio_callback, 
			uint32_t polling_interval_ms, 
			uint32_t gpio_number);
/**
 * @brief This API is used to stop the gpio notification.
 *
 * @return void.
 */
int mxuvc_gpio_notification_stop(void);

/**@}*/


/** @defgroup overlay_system Overlay Subsytem
  * @{
  */

/** structure containing image overlay specific information */
typedef struct
{
    /** width of overlay image */
    uint32_t width;
    /** height of overlay image */
    uint32_t height;
    /** x offset */
    uint32_t xoff;
    /** y offset */
    uint32_t yoff;
    /** index of overlay image */
    uint32_t  idx;
    /** */
    uint32_t  alpha;

} overlay_image_params_t;

/** structure containg text overlay specific information */
typedef struct
{
    /** x offset: must be multiple of 64/mbtile width.*/
    uint32_t xoff;
    /** y offset: must be multiple of 16/mbtile height.*/
    uint32_t yoff;
    /** index of overlay text image */
    uint32_t  idx;
} overlay_text_params_t;

/** structure containg text overlay specific information */
typedef struct
{
    /** font size: must be between 8 - 32*/
    uint32_t size;
    /** start font: first character used in font map*/
    uint32_t start;
    /** end font: last character used in font map */
    uint32_t  end;
    /** char width: width of each character in bytes. default 1 */
    uint32_t  width;
    /** char width: width of each character in bytes. default 1 */
    uint32_t  imgwidth;
    /** char width: width of each character in bytes. default 1 */
    uint32_t  imgheight;
} overlay_font_params_t;

/** structure containing overlay time specific information in hh:mm:ss format */
typedef struct
{
    /** hours – value: 0-23 */
    uint8_t hh;
    /** minutes – value: 0-59 */
    uint8_t mm;
    /** seconds – value: 0-59 */
    uint8_t ss;
    /** flag to enable or disable frame number update along with time */
    bool    frame_num_enable;

} overlay_time_t;

/** @defgroup overlay_init_deinit Init / Deinit
  * @{
  */

/**
 * @brief This API is used to initialize the USB vendor command system required
 * to transfer data between host and firmware. Common for image and text overlay.
 * Needed once as part of image or text overlay initialization.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_init(void);

/**
 * @brief This API is used to free all the resources allocated
 * by the overlay system. Common for image and text overlay.
 * Impacts both image and text apis.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_deinit(void);

/**@}*/

/** @defgroup overlay_control Overlay Parameters (Set/Get)
  * @{
  */

/* image overlay API's */

/**
 * @brief This API is used to add the YUV image at specific position on the given video channel. Maximum allowed size is 640x480.
 * 
 * @param[in] ch video channel on which logo is required to be overlaid
 * @param[in] params overlay image specific parameters like width, height, position offsets. alpha and index.
 * @param[in] yuv420p_filename path and filename of the YUV 420 file to be used for the image overlay
 * @param[in] alpha420p_filename path and filename of the Alpha 420 file to be used for the image overlay. 
 *            Optional parameter and can be set to NULL. When NULL per pixel alpha feature is disabled and params->alpha is used as global alpha. 
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_add_image(video_channel_t ch, overlay_image_params_t* params, char* yuv420p_filename, char* alpha420p_filename);
/**
 * @brief API to remove logo image on specific channel.
 * 
 * @param[in] ch video channel from which overlay image is to be removed
 * @param[in] idx index of the image to be removed. Possible values are 0 to 3.
 *                Index is provided as part of @ref mxuvc_overlay_add_text or
 *                @ref mxuvc_overlay_add_image
 *                
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_remove_image(video_channel_t ch, uint32_t idx);

/**
 * @brief NOT SUPPORTED
 * 
 * @param ch TBD
 * @param idx TBD
 * @param alpha TBD
 * 
 * @return TBD
 */
int mxuvc_overlay_set_transparency(video_channel_t ch, uint32_t idx, uint8_t alpha);

/**
 * @brief NOT SUPPORTED
 * 
 * @param ch TBD
 * @param idx TBD
 * @param colorkey TBD
 * 
 * @return TBD
 */
int mxuvc_overlay_set_colorkey(video_channel_t ch, uint32_t idx, uint32_t colorkey);
int mxuvc_overlay_load_font(video_channel_t ch, int font_size, char* font_file);

/**
 * @brief This API is used to add or update the text of specified text index
 * and at specified position. Firmware supports up to 5 texts to be overlaid
 * on the video.
 * 
 * @param[in] ch video channel to overlay text string.
 * @param[in] params overlay text parameters containg text position and index
 * @param[in] str updated text to overlay
 * @param[in] length length of the text in bytes.
 *            Firmware can support maximum string length of 24.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_add_text(video_channel_t ch, overlay_text_params_t* params, char *str, uint32_t length);

/**
 * @brief This API is used to remove the text from the specified text index.
 * 
 * @param[in] ch video channel from which text needs to be removed.
 * @param[in] idx index of the text to be used. Maximum of 5 indices are supported by the firmware.
 *                Possible values are 0 to 4.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_remove_text(video_channel_t ch, uint32_t idx);

/**
 * @brief This API is used to enable the overlay of the current time in the video
 * at a specified position. The time displayed is in 24 hour HH:MM:SS format
 * by default the start time is 00:00:00. When frame number is enabled,
 * the time is displayed in HH:MM:SS FF format. Where FF goes from 00 to 99.
 * 
 * @param[in] ch video channel on which time update needs to be enabled
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_show_time(video_channel_t ch);

/**
 * @brief This API is used to disable the overlay of the current time in the video.
 * 
 * @param[in] ch video channel on which time update needs to be disabled
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_hide_time(video_channel_t ch);

/**
 * @brief This API is used to update the start time of the overlay timer in the firmware.
 * 
 * @param[in] ch video channel on which initial value of time needs to be set
 * @param[in] params text overlay parameters like offset
 * @param[in] hhmmss start time to be set
 *                   Expected value of hour is from 0 to 23.
 *                   Expected value of minutes is from 0 to 59.
 *                   Expected value of seconds is from 0 to 59.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_set_time(video_channel_t ch, overlay_text_params_t* params, overlay_time_t* hhmmss);


/* text overlay API's */
/**
 * @brief This API is used to change the specific font on a video channel.
 * This api can be invoked anytime on the fly to allow for dynamic font changes.
 * This allows for each channel to have its own font type and size.
 * 
 * @param[in] ch video channel to overlay text string
 * @param[in] font configuration of the font used to overlay the text in the video.
 * @param[in] font_file filename (alongwith file path) of the font binary file
 *                      to be used for the text. The file provided should be in
 *                      accordance with the size specified. The text displayed would
 *                      be garbled otherwise.
 *                      
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_load_font_gpu(video_channel_t ch, overlay_font_params_t* font, char* font_file, char* alpha_channel);

/**
 * @brief This API is used to add or update the text of specified text index
 * and at specified position. Firmware supports up to 5 texts to be overlaid
 * on the video.
 * 
 * @param[in] ch video channel to overlay text string.
 * @param[in] params overlay text parameters containg text position and index
 * @param[in] str updated text to overlay
 * @param[in] length length of the text in bytes.
 *            Firmware can support maximum string length of 24.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_add_text_gpu(video_channel_t ch, overlay_text_params_t* params, char *str, uint32_t length);

/**
 * @brief This API is used to remove the text from the specified text index.
 * 
 * @param[in] ch video channel from which text needs to be removed.
 * @param[in] idx index of the text to be used. Maximum of 5 indices are supported by the firmware.
 *                Possible values are 0 to 4.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_remove_text_gpu(video_channel_t ch, uint32_t idx);

/**
 * @brief This API is used to enable the overlay of the current time in the video
 * at a specified position. The time displayed is in 24 hour HH:MM:SS format
 * by default the start time is 00:00:00. When frame number is enabled,
 * the time is displayed in HH:MM:SS FF format. Where FF goes from 00 to 99.
 * 
 * @param[in] ch video channel on which time update needs to be enabled
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_show_time(video_channel_t ch);

/**
 * @brief This API is used to disable the overlay of the current time in the video.
 * 
 * @param[in] ch video channel on which time update needs to be disabled
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_hide_time_gpu(video_channel_t ch);

/**
 * @brief This API is used to update the start time of the overlay timer in the firmware.
 * 
 * @param[in] ch video channel on which initial value of time needs to be set
 * @param[in] params text overlay parameters like offset
 * @param[in] hhmmss start time to be set
 *                   Expected value of hour is from 0 to 23.
 *                   Expected value of minutes is from 0 to 59.
 *                   Expected value of seconds is from 0 to 59.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_set_time_gpu(video_channel_t ch, overlay_text_params_t* params, overlay_time_t* hhmmss);

/**@}*/

/** @defgroup download_alpha_map download compressed alpha map
 * @{
 */

/**
 * @brief API to download compressed alpha map to the firmware for usage in overlay object.
 * @details This API allows to download a compressed alpha map from host to firmware. The firmware will decompress and apply the map to the video
 * 
 * @param ch video channel on which alpha map has to be applied
 * @param compressedalpha_filename This file is output of the based host tool which compresses the raster alpha map. This compression is in gz format
 *                      
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_download_compressed_alphamap(video_channel_t ch, char* compressedalpha_filename);

/**@}*/

/**@}*/

/** @defgroup custom_ctrl_system Custom-Control Subsytem
  * @{
  */

typedef enum {
    INTERNAL =0,
    EXTERNAL,
} audclk_mode_t ;

/** structure holds all the tracker values which are used for running
the voice activity detector in audio processing block running in camera.
It also has detector output to tell if any voice activity got detected or not. */
typedef struct {
    /** indicates if any voice activity is detected or not for the current audio frame */
    bool detector_decision;
    /** slow energy tracker value for the current audio frame */
    int slow_tracker;
    /** fast energy tracker value for the current audio frame */
    int fast_tracker;
    /** background energy tracker value for the current audio frame */
    int background_tracker;

}detector_metadata;

/** structure holds all the frame statistics which are generated after
post processing of each audio frame in the audio processing block in camera.
It indicates values like energy, pitch and other such values for the current
audio frame at the time of the request on @ref mxuvc_custom_control_get_audio_stats
is made. */
typedef struct {
    /** energy of the current audio frame */
    float ste;
    /** no of zero crossings in the current audio frame */
    float zcr;
    /** voice pitch of audio signal in the current audio frame */
    float pitch;
}frameStats_metadata;

/** structure is place holder for all kind of audio frame meta data generated
during processing of the each audio frame. The values get overwritten for
each audio frame. */
typedef struct {
    /** gain set by the AGC module */
    int gain;
    /** flag indicating if the bulk delay has been found between
    Near-End and Far-End data path for Echo canceller*/
    bool bulk_delay_found;
    /** no of samples processed before finding the bulk delay between 
    Near-End and Far-End. This is only valid when @ref bulk_delay_found is true. */
    int bulk_delay_samples;
    /** structure containing the parameters of voice activity detector */
    detector_metadata detector;
    /** structure containing the parameters of frame statistics */
    frameStats_metadata frameStats;
}asp_metadata;

/** ENUM for IR CUT filter mode, 
	according to the mode setting GPIO lines will be driven in firmware
    to drive the filter accordingly */
typedef enum{
    /** idle mode is the mode after power on */
    IRCF_IDLE= 0,
    /** enable mode */
    IRCF_ENABLE,
    /** disable mode */
    IRCF_DISABLE,
    /** when any programming failure happens in firmware then 
        IRCF_ERROR mode will be set. After reaching to 
        this state GC6500 needs a reset to recover */
    IRCF_ERROR
} ircf_state_t;

/** @defgroup custom_ctrl_init_deinit Init / Deinit
  * @{
  */

/**
 * @brief This API is used to initialize the custom control plugin.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_init(void);

/**
 * @brief This API is used to deallocate all the resources of the custom control plugin.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_deinit(void);

/**@}*/

/** @defgroup custom_ctrl_control  Custom-Control Parameters (Set/Get)
  * @{
  */

int mxuvc_custom_control_set_vad(uint32_t vad_status);

/**
 * @brief This API is used to set the camera audio capture data path
 * in the AEC mode where-in two audio inputs and the AEC preprocessing
 * filter are enabled in the audio capture data path as below, before
 * giving the samples to encode. One audio input is used to capture the
 * actual microphone data and the second input is used to capture the Far
 * end data which is played through the speaker on the host.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_enable_aec(void);

/**
 * @brief This API is used to set the camera audio capture data path in
 * the normal mode where in a single audio input is bypassed through an
 * echo cancellation and directly giving the samples to encode.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_disable_aec(void);

/**
 * @brief This API is used to configure samplerate of ADC. More descrption to be added.
 * 
 * @param[in] samplerate samplerate to set
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_set_audio_codec_samplerate(unsigned int samplerate);

/**
 * @brief This API is used to set the samplerate of the speaker in audio
 * playback datapath.
 * 
 * @param[in] samplerate sample rate of the audio signal to be set on to speaker.
 *                       For supported values please refer the datasheet of the
 *                       DAC present on your camera.
 *                       
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_set_spkr_samplerate(unsigned int samplerate);

/**
 * @brief This API is used to get the samplerate of the speaker in audio
 * playback datapath.
 * 
 * @param[out] samplerate sample rate of the audio playback data.
 *                       
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_get_spkr_samplerate(uint32_t *samplerate);

/**
 * @brief This API is used to set the speaker volume.
 * 
 * @param[in] volume volume level of speaker (Range - TBD)
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_set_spkr_gain(uint32_t volume);

/**
 * @brief This API is used to get the current speaker volume.
 * 
 * @param[out] volume volume level of speaker (Range - TBD)
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_get_spkr_gain(uint32_t *volume);

/**
 * @brief This API is used to enable the speaker on the camera.
 * This turns on the speaker.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_enable_spkr(void);

/**
 * @brief This API is used to disable the speaker on the camera.
 * This turns off the speaker.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_disable_spkr(void);

/**
 * @brief This API is used get the audio frame statistics from the camera
 * whenever required.
 * 
 * @param[out] audio_stats pointer to the structure @ref asp_metadata
 * containing audio statistics data computed for each frame. This api
 * will return the audio statistics of the latest audio frame captured.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_get_audio_stats(void * audio_stats);

/**
 * @brief This API is used to enable AGC on the camera.
 * This turns ON AGC module.
 * NOTE: This api will work only if asp is enabled in JSON.
 *
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_enable_agc(void);

/**
 * @brief This API is used to disable AGC on the camera.
 * This turns OFF the AGC module and set the MIC gain to user defined value.
 * NOTE: This api will work only if asp is enabled in JSON.
 *
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_disable_agc(void);

/**
 * @brief This API is used to turn On asp for the given channel.
 *
 * NOTE: Applicable only for PCM channel.
 *
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_enable_asp(audio_channel_t ch);

/**
 * @brief This API is used to turn Off asp for the given channel.
 *
 * NOTE: Applicable only for PCM channel.
 *
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_disable_asp(audio_channel_t ch);

int mxuvc_custom_control_get_audclk_mode(audio_channel_t ch, audclk_mode_t *mode);

int mxuvc_custom_control_set_audclk_mode(audio_channel_t ch, audclk_mode_t mode);

int mxuvc_custom_control_set_qparam(const char *object_name,
                                    const char *param_type,
                                    const char *param_name,
                                    int param_value,
                                    char activate_cfg);

/**
 * @brief This API is used to set the IFCF state in camera.
 * 
 * @param ircf_state_t state of IRCF
 * @param time_ms time delay between the state change in milli seconds.                     
 *
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_set_IRCF(ircf_state_t state, unsigned int time_ms);
/**
 * @brief This API is used to get the current IRCF state in camera.
 * 
 * @param ircf_state_t current state of IRCF
 *
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_custom_control_get_IRCF_state(ircf_state_t *state);     

/**@}*/

/* Debug functions */
/**
 * @brief TBD
 * 
 * @param outfile TBD
 * 
 * @return TBD
 */
int mxuvc_debug_startrec(char *outfile);

/**
 * @brief TBD
 *
 * @return TBD
 */
int mxuvc_debug_stoprec();

/**@}*/

/** @defgroup ldmap_system Dewarp Map Upload Subsytem
 * @{
 */

/** @defgroup ldmap_init_deinit Init / Deinit
 * @{
 */

/**
 * @brief This function initializes the map loading module and 
 * it must be called once before other functions in this set of API can be used. 
 * This is a module wise initializer. And only need to be called once regardless 
 * ofhow many dewarp objects there are.
 *  
 * @return 0 on Success, -1 on Failure
 */

int ldmap_init(void);

/**
 * @brief This function is called once when the map loading API is no longer needed. When called, it deallocates
 * resources that had been reserved for the Map Loading API.
 * 
 * @return Always return 0
 */
int ldmap_deinit(void);

/**@}*/

/** @defgroup map_upload Map Upload
 *   * @{
 *     */

/**
 * @brief This function uploads a map to dewarp object. This function can only be used once the dewarp has been
 * put in running state. Its job is to copy a map to the dewarp map queue, then increment the write pointer.
 * 
 * @param [in] pObjName This is a string that is used to identify the dewarp object the map is for. This string is same
 * as the dewarp name used by JSON script. Normally, it has names like “dewarp0”,
 * “dewarp1”.... Etc
 *
 * @param [in] pMapx An array of floats that contains the X components of the map array.
 * @param [in] pMapy An array of floats that contains the Y components of the map array.
 * @param [in] mapw Width of the map in number of points.
 * @param [in] maph Height of the map in number of points.
 * @param [in] inw Width of the input video. You can find this value in the JSON script
 * @param [in] inh Height of the input video. Same value as used in JSON script
 * @param [in] dww Width of the dewarped image. Get this from JSON script from parameter with the same
 * name. This is not the output size of the dewarp object.
 * @param [in] dwh Height of the dewarped image. Get this from JSON script from parameter with the same
 * name. This is not the output size of the dewarp object.
 * @return 0 on Success, -1 on Failure
 */
int ldmap_upload(const char *pObjName, float *pMapx, float *pMapy, short mapw, short maph, short inw, short inh, short dww, short dwh);


/**@}*/
/**@}*/

/** Privacy Mask APIs*/
/** ENUM to select specific shape geometry that needs to be rendered as privacy mask.*/
typedef enum {
    /** Rectangle */
    PRIVACY_SHAPE_RECT,
    /** Polygon */
    PRIVACY_SHAPE_POLYGON,
    PRIVACY_SHAPE_MAX,
} privacy_shape_type_t;

/** ENUM to select the specific mask region using on a single channel.*/
typedef enum {
    /** Mask ID 0 */
    PRIVACY_IDX_0 = 0,
    PRIVACY_IDX_1,
    PRIVACY_IDX_2,
    PRIVACY_IDX_3,
    PRIVACY_IDX_MAX,
} privacy_index_t;

/** Data structure to store a 2D rectangle.*/
typedef struct 
{
    /** X Offset */
    unsigned int                    xoff;
    /** Y Offset */
    unsigned int                    yoff;
    /** Width of rectangle */
    unsigned int                    width;
    /** Height of rectangle */
    unsigned int                    height;

}privacy_mask_shape_rect_t;

/** Data structure to define a coordinate in 2D plane */
typedef struct
{
    /** X coordinate */
    int                             x;
    /** Y coordinate */
    int                             y;
} privacy_mask_point_t;

/** Data structre to define an closed polygon in 2D plane */
typedef struct
{
    /**Number of points of a closed polygon.*/
    int                             num_points;
    /**Pointer to array of structure point_t of size  num_points. Points are allocated in clockwise direction where point[0] is nearest to the origin (0,0).*/
    privacy_mask_point_t*           points;     
}privacy_mask_shape_polygon_t;

/**Data structure to support various shapes to be used as privacy mask. To be used with compatible privacy mask shape type as per privacy_shape_type_t.*/
typedef union {
    /** Rectangle */
    privacy_mask_shape_rect_t    rect;
    /** Polygon */
    privacy_mask_shape_polygon_t polygon;
}privacy_shape_t;

/** Data structure to pass color and alpha. */
typedef struct {
    unsigned int                    yuva;
} privacy_color_t;

/** Data structure for passing various properties of privacy mask like mode and shape.*/
typedef struct {
    privacy_shape_type_t            type;
    privacy_shape_t*                shape;
    privacy_color_t*                color;
} privacy_params_t;

/**
 * @brief API to add a privacy mask on a specific video channel.
 * @details This API allows to add a privacy mask on the video channel using GPU hardware. Number of indexes per channel is controlled by PRIVACY_IDX_MAX 
 * and should be less than the same. mxuvc_overlay_privacy_remove_mask must be called before re-adding mask on same ch and idx values. Error will be returned 
 * if called consecutively without removing the mask.
 * 
 * @param ch video channel on which privacy mask needs to be applied.
 * @param params data structure containing properties of the mask like shape, type, color.
 * @param idx index to control a specific mask on same channel.
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_privacy_add_mask(video_channel_t ch, privacy_params_t* params, privacy_index_t idx);

/**
 * @brief API to remove a privacy mask on a specific video channel.
 * @details This API allows to remove a specific mask from the selected video channel. mxuvc_overlay_privacy_add_mask must be called before calling this api.
 * Error will be returned if called consecutively without adding the mask.
 * 
 * @param ch video channel from which privacy mask needs to be removed.
 * @param idx index of the mask that needs to removed.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_privacy_remove_mask(video_channel_t ch, privacy_index_t idx);

/**
 * @brief API to update the color and alpha of applied mask. Not Implemented Yet.
 * @details Not Implemented Yet. API allows to change the color and alpha of selected mask on specific video channel. mxuvc_overlay_privacy_add_mask must be called before calling 
 * this API else error will be returned. color is a 32 bit value with first 24 bit representing color in YUV format and last 8 bit representing alpha or 
 * transparency between 0x00-0xFF. Where 0xFF is opaque and 0x00 means transparent.
 * 
 * @param ch video channel from which color needs to be updated
 * @param colors colorkey in YUVA format.
 * @param idx index of the mask that needs to updated.
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_overlay_privacy_update_color(video_channel_t ch, privacy_color_t* colors, privacy_index_t idx);

/**
 * @brief This API is used to initialize the USB vendor command system required
 * to transfer data between host and firmware.
 * Needed once as part of initialization.
 * 
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_ispcfg_init(void);

/* ispcfg API's */
/**
 * @brief This API is used to download an ispcfg binary file.
 * This api can be invoked anytime on the fly to allow for dynamic changes.
 * 
 * @param[in] ispcfg_file filename (alongwith file path) of the ispcfg binary file
 *                      to be used.
 *                      
 * @return 0 on Success, -1 on Failure
 */
int mxuvc_ispcfg_load_file(char* ispcfg_file);

#ifdef __cplusplus
} // extern "C"
#endif

#endif  // #ifdef __MXUVC_H__
