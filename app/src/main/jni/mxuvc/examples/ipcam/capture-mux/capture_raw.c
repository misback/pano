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

//#define  CH1_CAPTURE    // Define it to capture CH1 in addition to raw channel
#define  PME_SAVE         // Define to save PME data to a file
#define  HISTOGRAM_SAVE   // Define to save histogram data to a file
//Following takes too much time for 1920x1080 resulting in frame losses
//as indicated by sequence mismatch error.
//So commented it out as default
//#define  MBSTATS_SAVE     // Define to save MB Stats data to a file.
#define  GLOBALSTATS_SAVE // Define to save global VPP Stats data to a file.
     
FILE *fd[NUM_MUX_VID_CHANNELS][NUM_VID_FORMAT];
        
static FILE *pme=0;
static FILE *pme_ov=0;      //File to test PME data with the tool to overlay PME data
                            //in the NV12 YUV file
static FILE *fMBStats=0;
static FILE *fHistogram=0;
static FILE *fGlobalStats=0;

static uint16_t nRawChW  = 0; // Raw Channel Width  in Pixels
static uint16_t nRawChH  = 0; // Raw Channel Height in Pixels
static int mbWidth       = 0; // Raw Channel #MBs Width
static int mbHeight      = 0; // Raw Channel #MBs Height
static int mbStride      = 0; // Raw Channel #MBs Stride
static int nMBStatSize   = 0; // Raw Channel MB Stats size per frame.
static int mbStatsOffset = 0; // Different Stats offset 

int *LumaAvgMBData;

#ifdef CH1_CAPTURE
static void ch_cb_CH1(unsigned char *buffer, unsigned int size,
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
#endif


#ifdef MBSTATS_SAVE

// STATS READING FUNCTION
int stat_read(unsigned char *inBuffer, int *outBufferA, int *outBufferB, int mode,
        int mbWidth, int mbHeight, int stride, int interlaced)
{
    int i, j, k, dataA, mb=0;

    for (i=0; i<mbHeight; i++) {

        if (interlaced)
            k = (2*stride*i) ;
        else
            k = (stride*i) ;

        // Incase of 16 bits/MB the stride should double
        if (mode==1 || mode==2)
            k = k * 2;

        for (j=0; j<mbWidth; j++, mb++) {
            
                // In case of 16 bits/MB we need to read two bytes and club into one
                // eg, LUMA SQUARED AVERAGE
                if (mode==2) {
                    dataA = inBuffer[k++];
                    dataA = (dataA << 8) | inBuffer[k++];
                    fprintf(fMBStats, "%4d ", dataA);
                }
                // In case of Luma Edge and Motion Edge the 1st byte is the edge direction
                // and the 2nd byte is the edge strength
                // eg, LUMA EDGE, LUMA MOTION EDGE

                else if(mode==1)  {
                    fprintf(fMBStats, "%4d ", inBuffer[k++]);
                    fprintf(fMBStats, "%4d ", inBuffer[k++]);
                } else {
                    fprintf(fMBStats, "%4d ", inBuffer[k++]);
                }            
        }

        fprintf(fMBStats, "\n");
  
    }
    fprintf(fMBStats, "\n");
    return 0;
}


void SaveMBStats(unsigned char *pBuff)
{
    unsigned char *inbuffer ;
    static int fcnt=0;
    int interlaced = 0;
    int v1[300]={0},v2[300]={0},field_num,i,j;

    int statsize = mbStride * mbHeight * (1+interlaced) ;

    for(field_num=0; field_num < (1+interlaced); field_num++)
    {
        inbuffer = pBuff + mbStride*field_num;

        fprintf(fMBStats, "FRAME: %d  \n", fcnt++);
        fprintf(fMBStats, "################ \n\n");

        // Luma MBAVG        
        fprintf(fMBStats, "Luma MBAVG  \n");
        stat_read( inbuffer + 0*statsize,   NULL, NULL, 0, mbWidth, mbHeight, mbStride, interlaced);

        // Luma HVLF
        fprintf(fMBStats,"Luma HVLF \n");
        stat_read( inbuffer + 1*statsize,  NULL, NULL, 0, mbWidth, mbHeight, mbStride, interlaced);

        // Luma HVHF
        fprintf(fMBStats,"Luma HVHF \n");
        stat_read(inbuffer + 2*statsize,  NULL, NULL, 0, mbWidth, mbHeight, mbStride, interlaced);

        // Luma Motion MBAvg
        fprintf(fMBStats,"Luma Motion MBAvg \n");
        stat_read(inbuffer + 3*statsize,  NULL, NULL, 0, mbWidth, mbHeight, mbStride, interlaced);

        // Luma Edge
        fprintf(fMBStats,"Luma Edge \n");
        stat_read(inbuffer + 4*statsize,  NULL,  NULL, 1, mbWidth, mbHeight, mbStride, interlaced);

        // Luma Motion Edge
        fprintf(fMBStats,"Luma Motion Edge \n");
        stat_read(inbuffer + 6*statsize,   NULL,  NULL, 1, mbWidth, mbHeight, mbStride, interlaced);

        // Chroma HHF MBAvg
        fprintf(fMBStats,"Chroma HHF MBAvg \n");
        stat_read(inbuffer + 8*statsize,   NULL, NULL, 0, mbWidth, mbHeight, mbStride, interlaced);

        // Chroma Motion MBAvg
        fprintf(fMBStats,"Chroma Motion MBAvg \n");
        stat_read(inbuffer + 9*statsize,   NULL, NULL, 0, mbWidth, mbHeight, mbStride, interlaced);

        // Color0 Sum
        fprintf(fMBStats,"Color0 Sum \n");
        stat_read(inbuffer +10*statsize,  NULL, NULL, 0, mbWidth, mbHeight, mbStride, interlaced);

        // Color1 Sum
        fprintf(fMBStats,"Color1 Sum \n");
        stat_read(inbuffer +11*statsize,   NULL, NULL, 0, mbWidth, mbHeight, mbStride, interlaced);

        // Color2 Sum
        fprintf(fMBStats,"Color2 Sum \n");
        stat_read(inbuffer +12*statsize,   NULL, NULL, 0, mbWidth, mbHeight, mbStride, interlaced);

        // Color3 Sum
        fprintf(fMBStats,"Color3 Sum \n");
        stat_read(inbuffer +13*statsize,  NULL, NULL, 0, mbWidth, mbHeight, mbStride, interlaced);

        // Luma Squared
        fprintf(fMBStats,"Luma Squared \n");
        stat_read(inbuffer +14*statsize,   NULL, NULL, 2, mbWidth, mbHeight, mbStride, interlaced);
    }

}

#endif   // MBSTATS_SAVE


#ifdef GLOBALSTATS_SAVE
void SaveGlobalStats(global_stats_t *pBuff)
{
    int i;
    static int fcnt=0;
    fprintf(fGlobalStats, "FRAME: %d  \n", fcnt++);
    fprintf(fGlobalStats, "################ \n\n");
    fprintf(fGlobalStats,"GPictYSum = %d\n", pBuff->GPictYSum);
    fprintf(fGlobalStats,"GPictCSum = %d\n", pBuff->GPictCSum);
    fprintf(fGlobalStats,"GPictYHorzHighFreq = %d\n", pBuff->GPictYHorzHighFreq);
    fprintf(fGlobalStats,"GPictYVertHighFreq = %d\n", pBuff->GPictYVertHighFreq);
    fprintf(fGlobalStats,"GPictCHighFreq = %d\n", pBuff->GPictCHighFreq);
    fprintf(fGlobalStats,"GPictYHighFreqSize = %d\n", pBuff->GPictYHighFreqSize);
    fprintf(fGlobalStats,"GPictCHighFreqSize = %d\n", pBuff->GPictCHighFreqSize);
    fprintf(fGlobalStats,"GPictYEdgeSize = %d\n", pBuff->GPictYEdgeSize);
    fprintf(fGlobalStats,"GPictEdgeStrengthMBSum = %d\n", pBuff->GPictEdgeStrengthMBSum);
    for(i=0;i<4;i++)
        fprintf(fGlobalStats,"GPictColorSum[%d] = %d\n", i, pBuff->GPictColorSum[i]);
    for(i=0;i<4;i++)
        fprintf(fGlobalStats,"GPictColorSize[%d] = %d\n", i, pBuff->GPictColorSize[i]);
}

#endif   // GLOBALSTATS_SAVE
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
            [VID_FORMAT_GREY_RAW]    = ".y8",
            [VID_FORMAT_NV12_RAW]    = ".i420",
    };
        static int frameCount=1;

    int row,col,addr,i;

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

#ifdef PME_SAVE
    if(info.pme.pmevtag != NULL)
    {
        int row, col, ref, dataA;
        int addr;
        int line, stride;
        int num_mvs;
        int mbaff;
        int num_refs;
        int partition;
        const int results2nummvs[] = { 1, 3, 4, 9 };
        char *pPmeData;

        if(pme == 0)
        {   
            pme=fopen("pme.txt", "wb");
            pme_ov=fopen("pme_overlay.txt", "wb");
            printf("PME file opened\n");
            //Using formatted output so that we can easily check data
        }
        num_mvs = results2nummvs[info.pme.pmevtag->ResultsMV];
        mbaff   = info.pme.pmevtag->PictStr == 1;
        num_refs= info.pme.pmevtag->NumRef ;
        line    = info.pme.pmevtag->mbWidth*num_mvs*2*(1+info.pme.pmevtag->MCost)*(1+mbaff)*info.pme.pmevtag->NumRef;
        stride  = (line+7)&~7;
        int inc = num_mvs*2*(1+info.pme.pmevtag->MCost)*(1+mbaff)*num_refs ; // This is one set of data per 8x8 macro block

//Writing to pme.txt

        fprintf(pme, "PME header in video frame:\n");
        fprintf(pme, "   PMEVectorSize %d\n",info.pme.pmevtag->PMEVectorSize);
        fprintf(pme, "   PMEVectorType = %d\n", info.pme.pmevtag->PMEVectorType);
        fprintf(pme, "   mbWidth = %d\n", info.pme.pmevtag->mbWidth);
        fprintf(pme, "   mbHeight = %d\n", info.pme.pmevtag->mbHeight);
        fprintf(pme, "   PictStr = %d\n", info.pme.pmevtag->PictStr);
        fprintf(pme, "   ResultsMV = %d\n", info.pme.pmevtag->ResultsMV);
        fprintf(pme, "   MCost = %d\n", info.pme.pmevtag->MCost);
        fprintf(pme, "   NumRef = %d\n", info.pme.pmevtag->NumRef);
        fprintf(pme, "   Refid0 = %d\n", info.pme.pmevtag->Refid0);
        fprintf(pme, "   Refid1 = %d\n", info.pme.pmevtag->Refid1);
        fprintf(pme, "   PMEMaxYRange = %d\n", info.pme.pmevtag->PMEMaxYRange);

        pPmeData = (char*)&info.pme.PMEVectorAddress[0];

        for(row = 0; row < info.pme.pmevtag->mbHeight; row++){
            fprintf(pme, "Row %d:\n", row);
            addr = row * stride;
            pPmeData = (char*)&info.pme.PMEVectorAddress[0] + addr;

            for(col = 0; col < line; col+=inc) {
                for (ref=0; ref < num_refs; ref++) {
                    fprintf(pme, "MV: ");
                    // parse and print vector pair
                    for(partition=0; partition< num_mvs; partition++) {

                        //mvx               
                        dataA = *pPmeData++;
                        fprintf(pme, "%2d ", dataA);
                        //mvy
                        dataA = *pPmeData++;
                        fprintf(pme, "%2d ", dataA);

                    }
                    if (info.pme.pmevtag->MCost) {
                        fprintf(pme, "\tMC: ");
                        // parse and print mcost 
                        for(partition=0; partition< num_mvs; partition++) {     
                            dataA = *pPmeData++;
                            dataA = (dataA << 8) | *pPmeData++;
                            fprintf(pme, "%4d ", dataA);
                        }           
                        fprintf(pme, "\n");
                    }
                    
                }

            }

            fprintf(pme, "\n");
        }

        fprintf(pme, "\n\n");

//Writing to pme_overlay.txt

        fprintf(pme_ov, "###Frame No%d\n", frameCount);
        pPmeData = &info.pme.PMEVectorAddress[0];

        for(row = 0; row < info.pme.pmevtag->mbHeight; row++)
        {
            addr = row * stride;
            pPmeData = (char*)&info.pme.PMEVectorAddress[0] + addr;
   
            for(col = 0; col < line; col+=inc)
            {
                for (ref=0; ref < num_refs; ref++) {
                    // parse and print vector pair
                    for(partition=0; partition< num_mvs; partition++) {
                        //Only write the first vector
                        //mvx                                      
                        dataA = *pPmeData++;
                        if(partition == 0)
                            fprintf(pme_ov, "%2d ", dataA);
                        //mvy
                        dataA = *pPmeData++;
                        if(partition == 0)
                            fprintf(pme_ov, "%2d\n", dataA);

                    }
                    if (info.pme.pmevtag->MCost) {
                        for(partition=0; partition< num_mvs; partition++) {     
                            pPmeData++;
                            pPmeData++;
                        }
                    }
                }
            }
        }
    }
        
#endif //PME_SAVE


#ifdef MBSTATS_SAVE
        //Save MB stats if available
        if (fMBStats == 0 && info.stats.buf) {
                fMBStats = fopen("mbstats.txt", "wt");
                printf("#%4d: stats size = %d\n", frameCount, info.stats.size);
        }
        if (fMBStats) {
            SaveMBStats((unsigned char *)info.stats.buf);
        }
#endif

#ifdef HISTOGRAM_SAVE
        if (fHistogram == 0 && info.histogram.buf) {
                fHistogram = fopen("histogram.bin", "wb");
                printf("#%4d: histogram size = %d\n", frameCount, info.histogram.size);
        }
        if (fHistogram) {
            fwrite(info.histogram.buf, sizeof(uint8_t), info.histogram.size, fHistogram);
        }
#endif

#ifdef GLOBALSTATS_SAVE
        if (fGlobalStats == 0 && info.globalstats.buf) {
                fGlobalStats = fopen("globalstats.txt", "wb");
                printf("#%4d: global stats size = %d\n", frameCount, info.globalstats.size);
        }
        if (fGlobalStats) {
            SaveGlobalStats(info.globalstats.buf);
        }
#endif

    frameCount++;

    fwrite(buffer, size, 1, fd[ch][fmt]);
    mxuvc_video_cb_buf_done(ch, info.buf_index);
}

static void close_fds() {
    int i, j;
    for(i=0; i<NUM_MUX_VID_CHANNELS; i++) {
        for(j=0; j<NUM_VID_FORMAT; j++) {
            if(fd[i][j])
                fclose(fd[i][j]);
        }
    }
    if(pme) {
        fclose(pme); pme = 0;
    }
    if(pme_ov) {
        fclose(pme_ov); pme_ov = 0;
    }
    if(fMBStats) {
        fclose(fMBStats); fMBStats = 0;
    }
    if(fHistogram) {
        fclose(fHistogram); fHistogram = 0;
    }
    if(fGlobalStats) {
        fclose(fGlobalStats); fGlobalStats = 0;
    }
}

int main(int argc, char **argv)
{
    int ret=0;
    int count=0;
    int ch_count = 0;
    long channel;
    long ch;

    /* Initialize camera */
    ret = mxuvc_video_init("v4l2","dev_offset=0;v4l_buffers_raw=4");

    if(ret < 0)
        return 1;
    ret = mxuvc_video_get_channel_count(&ch_count);
    printf("Total Channel count: %d\n",ch_count);
    ch = ch_count-1; // by default, we have RAW channel as the last channel in our design
    ret = mxuvc_video_get_resolution(ch, &nRawChW, &nRawChH);
    printf("Raw Channel: %4d x %4d\n", nRawChW, nRawChH);

    mbWidth  = (nRawChW + 15)/16;
    mbHeight = (nRawChH + 15)/16;
    mbStride = ((mbWidth + 7) & ~0x7);

    nMBStatSize = mbWidth * mbHeight;
    mbStatsOffset = mbStride * mbHeight;

    printf("MBW %d MBH %d MBS %d; MBStatSize=%d; mbStatOffset=%d\n", mbWidth, mbHeight, mbStride, nMBStatSize, mbStatsOffset);  

        LumaAvgMBData = malloc(sizeof(int)*nMBStatSize);
    
#ifdef CH1_CAPTURE
    /* Register callback functions */
    ret = mxuvc_video_register_cb(CH1, ch_cb_CH1, (void*)CH1);
    if(ret < 0)
        goto error;
#endif
    ret = mxuvc_video_register_cb(ch_count-1, ch_cb, (void*)ch);
    if(ret < 0)
        goto error;
    
    printf("\n");
    //get channel count of MUX channel
    ret = mxuvc_video_get_channel_count(&ch_count);
    printf("Total Channel count: %d\n",ch_count);
    //Commented following in order to use the default format set in firmware
    //ret = mxuvc_video_set_format(ch_count - 1, VID_FORMAT_NV12_RAW);

        STRUCT_Q_EPTZ_MODE_WM_ZCL eptzWM_ZclParam;
        eptzWM_ZclParam.Phi0   = 55;
        eptzWM_ZclParam.Rx     = 2;
        eptzWM_ZclParam.Ry     = 1;
        eptzWM_ZclParam.Rz     = 1;
        eptzWM_ZclParam.Gshift = 0;
        //mxuvc_video_set_dewarp_params((video_channel_t) ch_count - 1, 0, EMODE_WM_ZCL, (dewarp_params_t*) &eptzWM_ZclParam);
    //ret = mxuvc_video_set_resolution(ch_count - 1, 2688, 1520);
    //if(ret < 0)
    //  goto error;
#ifdef CH1_CAPTURE
        ret = mxuvc_video_start(CH1);
    if(ret < 0)
        goto error;
#endif
    usleep(5000);
 
    ret = mxuvc_video_start(ch_count - 1);
    if(ret < 0)
        goto error;

    usleep(5000);

    /* Main 'loop' */
    int counter;
    if (argc > 1){
        counter = atoi(argv[1]);
    } else
        counter = 15;

    while(counter--) {
        if(!mxuvc_video_alive()) {
            goto error;
        }
        printf("\r%i secs left", counter+1);


        fflush(stdout);
        sleep(1);
    }
#ifdef CH1_CAPTURE
    ret = mxuvc_video_stop(CH1);
    if(ret < 0)
        goto error;
#endif
    ret = mxuvc_video_stop(ch_count - 1);
    if(ret < 0)
        goto error;
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
