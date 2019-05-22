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

#define MAX_NUM_PANELS_PER_CHANNEL 6  //Should match with value defined in firmware

/* 
   Get dewarp metadata for all panels.
   Might be possible that not all panels are active so do not assume
   that dewarp_info[] is filled in panel order with no missing panel.
*/
void getDewarpMetadata(metadata_t *metadata, QMedExtDewarpStruct *dewarp_info[], int maxCount, int *numPanels)
{
    unsigned int cur_offset = 0;
    int count=0;

    *numPanels = 0;
    while(cur_offset < metadata->total_len) 
    { 
        QMedExtStruct *extension = 
            (QMedExtStruct*)(metadata->qmedExt+cur_offset); 
        unsigned long size = BE32(extension->boxSize);
        unsigned long type = BE32(extension->boxType);
        cur_offset += size;
            
        if(type == QMED_EXT_TYPE_DEWARP)
        { 
            dewarp_info[count] = (QMedExtDewarpStruct *) extension;
            (*numPanels)++;
            count++;
            if(count == maxCount)
                break;
        } 
    }
}

static void ch_cb(unsigned char *buffer, unsigned int size,
                  video_info_t info, void *user_data)
{
    static QMedExtDewarpStruct prev_dewarp_info[NUM_IPCAM_VID_CHANNELS][MAX_NUM_PANELS_PER_CHANNEL];
    int count;
    int numPanels=0;

    video_format_t fmt = (video_format_t) info.format;
    video_channel_t ch = (video_channel_t) user_data;
    metadata_t *metadata = &(info.metadata);
    QMedExtDewarpStruct *dewarp_info[MAX_NUM_PANELS_PER_CHANNEL];

    if (fmt < FIRST_VID_FORMAT || fmt >= NUM_VID_FORMAT) 
    {
        printf("Unknown Video format\n");
        return;
    }


    getDewarpMetadata(metadata, dewarp_info, MAX_NUM_PANELS_PER_CHANNEL, &numPanels); 

    //printf("numPanels=%d\n", numPanels);
    for(count=0;count<numPanels;count++)
    {
        //Do not assume that count is panel number as there might be panel skip in metadata due to inactive panels
        
        //Compare with previous values
        int i;
        int changed=0;
        if(dewarp_info[count]->mapType != prev_dewarp_info[ch][dewarp_info[count]->panel].mapType)
            changed = 1;
        else 
        {
            for(i=0;i<sizeof(dewarp_info[count]->params)/sizeof(int); i++) 
            {
                if(dewarp_info[count]->params[i] != prev_dewarp_info[ch][dewarp_info[count]->panel].params[i])
                {
                    changed=1;
                    break;
                }
            }
        }
        if(changed)
        {
            prev_dewarp_info[ch][dewarp_info[count]->panel] = *dewarp_info[count];
            printf("dewarp_metadata changed on channel CH%d Panel %d:\n"
                   "    mapType: %d  divisor=%d\n"
                   "    Params: ",
                   ch+1, dewarp_info[count]->panel, dewarp_info[count]->mapType, dewarp_info[count]->divisor);

            for(i=0;i<sizeof(dewarp_info[count]->params)/sizeof(int); i++)
                printf("%d ", dewarp_info[count]->params[i]);

            printf("\n\n");
        }
    }

    mxuvc_video_cb_buf_done(ch, info.buf_index);
}

void print_format(video_format_t fmt)
{
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
    int goplen = 0;
    int value, comp_q;
    noise_filter_mode_t sel;
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
    if (argc > 1) 
        counter = atoi(argv[1]);
    else
        counter = 15;
    int chid =CH1;
    //while (counter--)
    while (1) 
    {
        if (!mxuvc_video_alive()) 
        {
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
