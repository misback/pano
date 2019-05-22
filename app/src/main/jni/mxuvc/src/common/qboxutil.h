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

/*******************************************************************************
* @(#) $Id: qboxutil.h 18631 2008-12-20 03:41:08Z echung $
*******************************************************************************/
#ifndef QBOX_UTIL_HH
#define QBOX_UTIL_HH

#ifndef QBOX_SAMPLE_FLAGS_SYNC_POINT
#define QBOX_SAMPLE_FLAGS_SYNC_POINT 0x4
#endif

#ifndef QBOX_SAMPLE_FLAGS_CONFIGURATION_INFO
#define QBOX_SAMPLE_FLAGS_CONFIGURATION_INFO 0x1
#endif

#ifndef QBOX_FLAGS_LAST_SAMPLE
#define QBOX_FLAGS_LAST_SAMPLE 0x2
#endif

typedef enum {
    QBOX_SAMPLE_TYPE_UNKNOWN,
    QBOX_SAMPLE_TYPE_AUDIO,
    QBOX_SAMPLE_TYPE_VIDEO,
    QBOX_SAMPLE_TYPE_JPG,
} QBOX_SAMPLE_TYPE;

#define SAMPLE_TYPE_QAC 0x1
#define SAMPLE_TYPE_H264 0x2
#define SAMPLE_TYPE_PCM 0x3
#define SAMPLE_TYPE_DEBUG 0x4
#define SAMPLE_TYPE_H264_SLICE 0x5
#define SAMPLE_TYPE_QMA 0x6
#define SAMPLE_TYPE_USER_METADATA 0xf

int GetQBoxVersionNum();
int GetQBoxHdrSize(int version);
int GetQBoxSize(char *hdr);
int GetQBoxVersion(char *hdr);
QBOX_SAMPLE_TYPE GetQBoxSampleType(char *hdr);
int GetQBoxExactSampleType(char *hdr);
int GetQBoxSampleSize(char *hdr);
int GetQBoxBoxFlags(char *hdr);
int GetQBoxFlagsPadding(char *hdr);
int GetQBoxDataAddr(char *hdr);
int GetQBoxStreamID(char *hdr);
void SetQBoxFlagsPadding(char *hdr);
void SetQBoxSize(char *hdr, unsigned long size);
void SetQBoxType(char *hdr);
void SetQBoxVersion(char *hdr, unsigned char v);
void SetQBoxFlags(char *hdr, unsigned long f);
void SetQBoxSampleStreamType(char *hdr, unsigned short type);
void SetQBoxSampleStreamId(char *hdr, unsigned short id);
void SetQBoxSampleFlags(char *hdr, unsigned long flags);
void SetQBoxSampleCTS64(char *hdr, unsigned int h, unsigned int l);
int QBoxIsVideoMeta(char *hdr);
int QBoxIsMP2AudioMeta(char *hdr);
int GetQBoxType(char *hdr);
int QBoxParseVideoMeta(char *m, int *width, int *height, int *gop, int *frameticks);
int QBoxParseAudioMeta(char *m, int *samplerate, int *samplesize, int *channels);
int GetQBoxCTS(char *hdr);
uint32_t GetQBoxCTS64(char *hdr);
int GetQBoxSampleFlags(char *hdr);



#endif  /* QBOX_UTIL_HH */
/*******************************************************************************
* vi: set shiftwidth=4 tabstop=8 softtabstop=4 expandtab nosmarttab:
*******************************************************************************/
