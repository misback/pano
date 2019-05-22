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
#include <unistd.h>
#include <pthread.h>

#include "mxuvc.h"
#include "qmedext.h"

static void ch_cb(unsigned char *buffer, unsigned int size,
                  video_info_t info, void *user_data)
{
    static unsigned int unclippedEV_prev[NUM_IPCAM_VID_CHANNELS];         //Should be initialized to 0 by compiler
    static QMedExtAwbStruct prev_awb_info[NUM_IPCAM_VID_CHANNELS];        //Should be initialized to 0 by compiler

    video_format_t fmt = (video_format_t) info.format;
    video_channel_t ch = (video_channel_t) user_data;

    if (fmt < FIRST_VID_FORMAT || fmt >= NUM_VID_FORMAT) {
        printf("Unknown Video format\n");
        return;
    }

    metadata_t *metadata = &(info.metadata);
    QMedExtSensorStruct *sensor_info;
    //Don't enable following two AWB related metadata if not needed
    QMedExtAwbStruct *awb_info;
    QMedExtAwbInputStatsStruct *awbinputstats_info;
    //Enable following if needed
    QMedExtFullAEHistogramStruct *aehistogram_info;

    METADATA_GET(metadata, sensor_info, QMedExtSensorStruct, QMED_EXT_TYPE_SENSOR);
    METADATA_GET(metadata, awb_info, QMedExtAwbStruct, QMED_EXT_TYPE_AWB);
    METADATA_GET(metadata, awbinputstats_info, QMedExtAwbInputStatsStruct, QMED_EXT_TYPE_AWBINPUTSTATS);
    METADATA_GET(metadata, aehistogram_info, QMedExtFullAEHistogramStruct, QMED_EXT_TYPE_FULLAEHISTOGRAM);

    if (sensor_info) {
        if(sensor_info->unclippedEV_q24_8 != unclippedEV_prev[ch]) {
            unclippedEV_prev[ch] = sensor_info->unclippedEV_q24_8;
            printf("Sensor Info on channel CH%i\n", ch+1);
            printf("Unclipped EV          = %.2f\n",
                   ((float)sensor_info->unclippedEV_q24_8) / 256);
            printf("Total gain            = %.2f\n", 
                   ((float)sensor_info->totalGain_q24_8) / 256);
            printf("Integration Rows      = %u\n"  , sensor_info->integrationRows);
            printf("VTS                   = %u\n"  , sensor_info->vts);
            printf("Integration Time (ms) = %.2f\n\n", 
                   ((float)sensor_info->integrationTime_q24_8) / 256);
        }
    }

    if (awb_info) {
        //Compare with previous values - change logic according to your needs
        int changed=0;
        // SH: When AWB is disabled, current and previous gains will be the 
        // same. To enable reading statistics when AWB is disabled, set/force
        // "changed" to 1.
        if((awb_info->awbFinalRG != prev_awb_info[ch].awbFinalRG) ||
           (awb_info->awbFinalGG != prev_awb_info[ch].awbFinalGG) ||
           (awb_info->awbFinalBG != prev_awb_info[ch].awbFinalBG))
        changed=1;
        if(changed)
        {
            prev_awb_info[ch] = *awb_info;
            printf("AWB Info on channel CH%i\n", ch+1);
            printf("AWB final gain:  (R) 0x%04x  (G) 0x%04x  (B) 0x%04x\n",
                      awb_info->awbFinalRG, awb_info->awbFinalGG, awb_info->awbFinalBG); 
            printf("Color temperature: %d\n", awb_info->awbColorTemperature);
            if (awbinputstats_info)
            {
                int x,y,index;
                printf("Zone Stats:\n");
                printf("rg:\n");
                index=0;
                for(y=0;y<ISP_METERING_ZONES_Y;y++)
                {
                    for(x=0;x<ISP_METERING_ZONES_X;x++)
                    {
                        printf("0x%04x  ", awbinputstats_info->awbZoneStats[index].rg);
                        index++;
                    }
                    printf("\n");
                }
                printf("\n");

                printf("bg:\n");
                index=0;
                for(y=0;y<ISP_METERING_ZONES_Y;y++)
                {
                    for(x=0;x<ISP_METERING_ZONES_X;x++)
                    {
                        printf("0x%04x  ", awbinputstats_info->awbZoneStats[index].bg);
                        index++;
                    }
                    printf("\n");
                }
                printf("\n");

                printf("sum:\n");
                index=0;
                for(y=0;y<ISP_METERING_ZONES_Y;y++)
                {
                    for(x=0;x<ISP_METERING_ZONES_X;x++)
                    {
                        printf("0x%04x  ", awbinputstats_info->awbZoneStats[index].sum);
                        index++;
                    }
                    printf("\n");
                }
                printf("\n");
            }
        }
    }
    if (aehistogram_info) 
    {
        int i;
        printf("Full AE histogram:\n");
        for(i = 0; i < FULL_HISTOGRAM_BANDS; i++)
        {
            printf("%10d  ", aehistogram_info->fullHistogram[i]); 
            if(((i+1) % 8) == 0)    //Line break after 8 values for better formatting
                printf("\n");
        }
        printf("\n\n\n\n");
    }

    mxuvc_video_cb_buf_done(ch, info.buf_index);
}

void print_format(video_format_t fmt) {
    switch (fmt) {
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

int main(int argc, char **argv)
{
    int ret=0;
    int count=0;
    video_format_t fmt;
    int ch_count = 0;
    long channel;
    uint16_t width, hight;
    int framerate = 0;
    int goplen = 0;
    int value, comp_q;
    noise_filter_mode_t sel;
    pwr_line_freq_mode_t freq;
    zone_wb_set_t wb;
    camer_mode_t mode;
    /* Initialize camera */
    ret = mxuvc_video_init("v4l2","dev_offset=0");

    if (ret < 0)
        return 1;

    ret = mxuvc_get_camera_mode(&mode);
    if (ret < 0)
        return 1;
    printf("Camera mode %s\n",mode==IPCAM ? "IPCAM":"SKYPE");

    /* Register callback functions */
    ret = mxuvc_video_register_cb(CH1, ch_cb, (void*)CH1);
    if (ret < 0)
        goto error;

    printf("\n");
    //get channel count of MUX channel
    ret = mxuvc_video_get_channel_count(&ch_count);
    printf("Total Channel count: %d\n",ch_count);
    //remove raw channel from count
    ch_count = ch_count - 1;

    for (channel=CH2 ; channel<ch_count; channel++)
    {
        ret = mxuvc_video_register_cb(channel, ch_cb, (void*)channel);
        if (ret < 0)
            goto error;
    }

    printf("\n");
    for (channel=CH1; channel<ch_count ; channel++) {
        /* Start streaming */
        ret = mxuvc_video_start(channel);

        if (ret < 0)
            goto error;
    }

    usleep(5000);

    /* Main 'loop' */
    int counter;
    if (argc > 1) {
        counter = atoi(argv[1]);
    } else
        counter = 15;
    int chid =CH1;
    //while (counter--) {
    while (1) {
        if (!mxuvc_video_alive()) {
            goto error;
        }
        //printf("\r%i secs left\n", counter+1);
        // mxuvc_video_set_dewarp_params((video_channel_t) chid, 0, EMODE_WM_ZCL, (dewarp_params_t*) &eptzWM_ZclParam);
        chid++;
        fflush(stdout);
        sleep(1);
    }

    for (channel=CH1; channel<ch_count ; channel++)
    {
        /* Stop streaming */
        ret = mxuvc_video_stop(channel);
        if (ret < 0)
            goto error;
    }
    /* Deinitialize and exit */
    mxuvc_video_deinit();


    printf("Success\n");

    return 0;
error:
    mxuvc_video_deinit();
    printf("Failure\n");
    return 1;
}
