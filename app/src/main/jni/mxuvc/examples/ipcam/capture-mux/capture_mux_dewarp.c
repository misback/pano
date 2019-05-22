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

#define START_FIRST  1            //1: Start the channel first and then do dewarp APIs
#define PANEL_SELECT 0            //1: Use PMODE_SELECT/PMODE_UNSELECT for selecting/unselecting a panel instead of sending individual commands to each panel

#define MAX_NUM_VCH     8         //Maximum nnumber of video channels

#define COMPOSITOR_CH   CH1       //Compositor channel (if enabled)

FILE *fd[NUM_MUX_VID_CHANNELS][NUM_VID_FORMAT];

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
        [VID_FORMAT_METADATAS]   = ".datas",
    };

    video_format_t fmt = (video_format_t) info.format;
    video_channel_t ch = (video_channel_t) user_data;

    if (fmt < FIRST_VID_FORMAT || fmt >= NUM_VID_FORMAT) {
        printf("Unknown Video format\n");
        return;
    }

    if (fd[ch][fmt] == NULL) {
        char *fname = malloc(strlen(basename) + strlen(ext[fmt]) + 1);
        strcpy(fname, basename);
        strcpy(fname + strlen(fname), ext[fmt]);
        fname[6] = ((char) ch + 1) % 10 + 48;
        fd[ch][fmt] = fopen(fname, "w");
    }


    fwrite(buffer, size, 1, fd[ch][fmt]);
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

static void close_fds() {
    int i, j;
    for (i=0; i<NUM_MUX_VID_CHANNELS; i++) {
        for (j=0; j<NUM_VID_FORMAT; j++) {
            if (fd[i][j])
                fclose(fd[i][j]);
        }
    }
}

void print_dewarp_info(video_channel_t ch, int panel, dewarp_mode_t mode,
                       dewarp_params_t* param)
{
    int size = 0;
    printf("CH%d dewarp info: Mode ",ch+1);
    switch (mode) {
    case EMODE_OFF:
        printf("EMODE_OFF \n");
        break;

    case EMODE_WM_ZCL:
        size = sizeof (STRUCT_Q_EPTZ_MODE_WM_ZCL) / sizeof (int);
        printf("EMODE_WM_ZCL ");

        break;

    case EMODE_WM_ZCLCylinder:
        size = sizeof (STRUCT_Q_EPTZ_MODE_WM_ZCLCYLINDER) / sizeof (int);
        printf("EMODE_WM_ZCLCylinder \n");
        break;

    case EMODE_WM_ZCLStretch:
        size = sizeof (STRUCT_Q_EPTZ_MODE_WM_ZCLSTRETCH) / sizeof (int);
        printf("EMODE_WM_ZCLStretch \n");
        break;

    case EMODE_WM_1PanelEPTZ:
        size = sizeof (STRUCT_Q_EPTZ_MODE_WM_1PANELEPTZ) / sizeof (int);
        printf("EMODE_WM_1PanelEPTZ \n");
        break;

    case EMODE_WM_Sweep_1PanelEPTZ:
        size = sizeof (STRUCT_Q_EPTZ_MODE_SWEEP_WM_1PANELEPTZ) / sizeof (int);
        printf("EMODE_WM_Sweep_1PanelEPTZ \n");
        break;

    case EMODE_WM_Magnify:
        size = sizeof (STRUCT_Q_MODE_MAGNIFY) / sizeof (int);
        printf("EMODE_WM_Magnify \n");
        break;

    default:
        size = 0;
        printf("ERR: Unsupported dewarp mode %d\n",mode);
    }

    int i;
    for (i = 0; i < size; i++) {

        printf("Param[%d] = %d\n", i, *((int*)param + i));

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
    int fov;
    int mode = 0;
    config_params_t c[MAX_NUM_VCH];

    /* Initialize camera */
    ret = mxuvc_video_init("v4l2","dev_offset=0");

    if (ret < 0)
        return 1;

    /* Register callback functions */
    ret = mxuvc_video_register_cb(CH1, ch_cb, (void*)CH1);
    if (ret < 0)
        goto error;

    if (argc > 2) {
        mode = atoi(argv[2]);
        printf("Using Dewarp Mode: %d\n",mode);
    } else {
        mode = EMODE_WM_ZCLStretch;
        printf("Using default Mode: %d\n",mode);
    }


    ret = mxuvc_video_get_channel_count(&ch_count);
    printf("Total Channel count: %d\n",ch_count);   //This always adds raw channel count even if not there

    for (channel=CH1 ; channel<ch_count; channel++) {
        dewarp_params_t Param;
        int panel=0;
        int k = 0;
        dewarp_mode_t mode;

        mxuvc_video_get_config_params(channel, &c[channel]);

        for (k = 0; k < sizeof (config_params_t)/ sizeof (int); k++)
            printf("CH%ld: Config Param[%d] = %d \n", channel+1, k , ((int*)&c[channel])[k]);

        if (c[channel].dewarp)
        {
            ret = mxuvc_video_get_dewarp_params(channel, panel, &mode, (dewarp_params_t *)&Param);
            if (ret < 0)
                goto error;
            print_dewarp_info(channel, panel, mode, &Param);
        }
    }

    dewarp_params_t p;

    memset(&p, 0, sizeof(p));

    switch (mode)
    {
    case EMODE_OFF:
    {
        break;
    }
    case EMODE_WM_ZCL:
    {
        p.eptz_mode_wm_zcl.Phi0   = 55;
        p.eptz_mode_wm_zcl.Rx     = 2;
        p.eptz_mode_wm_zcl.Ry     = 1;
        p.eptz_mode_wm_zcl.Rz     = 1;
        p.eptz_mode_wm_zcl.Gshift = 0;
        break;
    }
    case EMODE_WM_ZCLCylinder:
    {
        p.eptz_mode_wm_zclcylinder.CylinderHeightMicro = (int)(0.95*1000000);
        break;
    }
    case EMODE_WM_ZCLStretch:
    {
        p.eptz_mode_wm_zclstretch.HPan = 0;
        p.eptz_mode_wm_zclstretch.VPan = 0;
        p.eptz_mode_wm_zclstretch.Zoom = 1;
        break;
    }
    case EMODE_WM_1PanelEPTZ:
    {
        p.eptz_mode_wm_1paneleptz.HPan = -50;
        p.eptz_mode_wm_1paneleptz.VPan = 0;
        p.eptz_mode_wm_1paneleptz.Zoom = 40;
        p.eptz_mode_wm_1paneleptz.Tilt = 0;
        break;
    }
    case EMODE_WM_Sweep_1PanelEPTZ:
    {
        p.eptz_mode_sweep_wm_1panelptz.HPanStart = -10;
        p.eptz_mode_sweep_wm_1panelptz.VPanStart = 0;
        p.eptz_mode_sweep_wm_1panelptz.ZoomStart = 40;
        p.eptz_mode_sweep_wm_1panelptz.TiltStart = 0;
        p.eptz_mode_sweep_wm_1panelptz.HPanInc = 1;
        p.eptz_mode_sweep_wm_1panelptz.TiltInc = 1;
        p.eptz_mode_sweep_wm_1panelptz.VPanInc = 1;
        p.eptz_mode_sweep_wm_1panelptz.ZoomInc = 1;
        p.eptz_mode_sweep_wm_1panelptz.Period  = 40;
        break;
    }
    case EMODE_WM_Magnify:
    {
        p.eptz_mode_magnify.zoom = 1;
        p.eptz_mode_magnify.radius = 100;
        p.eptz_mode_magnify.xCenter = 100;
        p.eptz_mode_magnify.yCenter = 100;
        break;
    }
    default:
        printf("Dewarp: Invalid EPTZ mode %d\n", mode);
        exit(1);
        break;
    }
#if START_FIRST
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
#endif
    
    //Issue compositor APIs only if enabled - also this sample code assumes in most places that it is 2x2
    if(c[COMPOSITOR_CH].composite)
    {
        panel_params_t panelparams;

        printf("Getting compositor parameters for channel CH%d\n", COMPOSITOR_CH+1);
        for (count=0;count<c[COMPOSITOR_CH].panels;count++)
        {
            mxuvc_video_get_compositor_params(COMPOSITOR_CH, count, (panel_mode_t*)&mode, &panelparams);
            printf("panel %d: mode=%d x=%-3d y=%-3d width=%-4d height=%-4d\n",
                count, mode, panelparams.x, panelparams.y, panelparams.width, panelparams.height);
        }

        //Zooom out panel 1 to full view
        panelparams.x = 0;
        panelparams.y = 0;
        panelparams.width = 1280;   //Assuming dewarp display width is 1280
        panelparams.height = 720;   //Assuming dewarp display height is 720
#if PANEL_SELECT
        mxuvc_video_set_compositor_params(COMPOSITOR_CH, 1, PMODE_SELECT, &panelparams);
#else
        //panelparams ignored in firmware when PMODE_OFF is used
        mxuvc_video_set_compositor_params(COMPOSITOR_CH, 0, PMODE_OFF, &panelparams);
        mxuvc_video_set_compositor_params(COMPOSITOR_CH, 2, PMODE_OFF, &panelparams);
        mxuvc_video_set_compositor_params(COMPOSITOR_CH, 3, PMODE_OFF, &panelparams);
        mxuvc_video_set_compositor_params(COMPOSITOR_CH, 1, PMODE_ON, &panelparams);
#endif
        
    }


    mode = EMODE_WM_ZCL;
    p.eptz_mode_wm_zcl.Phi0   = 55;
    p.eptz_mode_wm_zcl.Rx     = 2;
    p.eptz_mode_wm_zcl.Ry     = 1;
    p.eptz_mode_wm_zcl.Rz     = 1;
    p.eptz_mode_wm_zcl.Gshift = 0;

    if(c[CH2].dewarp)
    {
        mxuvc_video_set_dewarp_params(CH2, 0, mode, &p);
        mxuvc_video_set_dewarp_params(CH2, 1, mode, &p);
        mxuvc_video_set_dewarp_params(CH2, 2, mode, &p);
        mxuvc_video_set_dewarp_params(CH2, 3, mode, &p);
    }

    mode = EMODE_WM_Sweep_1PanelEPTZ;
    p.eptz_mode_sweep_wm_1panelptz.HPanStart = -10;
    p.eptz_mode_sweep_wm_1panelptz.VPanStart = 0;
    p.eptz_mode_sweep_wm_1panelptz.ZoomStart = 40;
    p.eptz_mode_sweep_wm_1panelptz.TiltStart = 0;
    p.eptz_mode_sweep_wm_1panelptz.HPanInc = 1;
    p.eptz_mode_sweep_wm_1panelptz.TiltInc = 1;
    p.eptz_mode_sweep_wm_1panelptz.VPanInc = 1;
    p.eptz_mode_sweep_wm_1panelptz.ZoomInc = 1;
    p.eptz_mode_sweep_wm_1panelptz.Period  = 40;

    if(c[CH3].dewarp)
    {
        int panel;
        
        for(panel = 0; panel < c[CH3].panels; panel++)
        {
            mxuvc_video_set_dewarp_params(CH3, panel, mode, &p);
        }
        for(panel = 0; panel < c[CH3].panels; panel++)
        {
            mxuvc_video_get_dewarp_params(CH3, panel, (dewarp_mode_t*)&mode, &p);
            print_dewarp_info(CH3, panel, mode, &p);
        }
    }

#if !START_FIRST
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
#endif
    /* Main 'loop' */
    int counter;
    int panelRestoreCount;   //Count at which panel will be restored

    if (argc > 1) {
        counter = atoi(argv[1]);
    } else
        counter = 15;
    panelRestoreCount = counter/2;

    while (counter--) {
        if (!mxuvc_video_alive()) {
            goto error;
        }
        printf("\r%i secs left\n", counter+1);
        fflush(stdout);
        sleep(1);
        if (counter  == panelRestoreCount && c[COMPOSITOR_CH].composite)
        {
            panel_params_t panelparams;
            printf("Restoring to 2x2 panel view\n");
#if PANEL_SELECT
            mxuvc_video_set_compositor_params(COMPOSITOR_CH, 1, PMODE_UNSELECT, &panelparams);
#else
            panelparams.x = 640;
            panelparams.y = 0;
            panelparams.width = 640;
            panelparams.height = 360;
            mxuvc_video_set_compositor_params(COMPOSITOR_CH, 1, PMODE_ON, &panelparams);
            panelparams.x = 0;
            panelparams.y = 0;
            panelparams.width = 640;
            panelparams.height = 360;
            mxuvc_video_set_compositor_params(COMPOSITOR_CH, 0, PMODE_ON, &panelparams);
            panelparams.x = 0;
            panelparams.y = 360;
            panelparams.width = 640;
            panelparams.height = 360;
            mxuvc_video_set_compositor_params(COMPOSITOR_CH, 2, PMODE_ON, &panelparams);
            panelparams.x = 640;
            panelparams.y = 360;
            panelparams.width = 640;
            panelparams.height = 360;
            mxuvc_video_set_compositor_params(COMPOSITOR_CH, 3, PMODE_ON, &panelparams);
#endif
        }

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

    close_fds();

    printf("Success\n");

    return 0;
error:
    mxuvc_video_deinit();
    close_fds();
    printf("Failure\n");
    return 1;
}
