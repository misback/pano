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
// ## Release Control Block
// ## Level 1

#ifndef __QMED_HH
#define __QMED_HH

/* QMED declarations */

#define QMED_VERSION_NUM 0

//////////////////////////////////////////////////////////////////////////////
// version: 0
//
// SDL
// aligned(8) class QMed extends FullBox('qmed', version = 0, boxflags) {
//     unsigned short major_media_type;
//     unsigned short minor_media_type;
//     unsigned char media_data[];
// }
//
// equivalent to 
// typedef struct {
//     unsigned long box_size;
//     unsigned long box_type; // "qmed"
//     unsigned long box_flags; // (version << 24 | boxflags)
//     unsigned short major_media_type;
//     unsigned short minor_media_type;
//     unsigned char media_data[];
// } QMed;
//
// version 0 does not use large box
//
// box_flags
// 31 - 24         23 - 0
// version         boxflags
//
// major_media_type:
// 0x01 AAC audio. media_data contains audio stream information
// 0x02 H.264 video. media_data contains audio frame information
// 0x05 H.264 video. sample_data contains video slice or configuration info.
// 0x06 MP1 audio. media_data contain contains stream information
// 0x09 G.711 audio. sample_data contains one audio frame
//
// minor_media_type:
//
//////////////////////////////////////////////////////////////////////////////

#define QMED_BOX_TYPE 0x716d6564
#define QMED_MAJOR_MEDIA_TYPE_AAC 0x1
#define QMED_MAJOR_MEDIA_TYPE_H264 0x2
#define QMED_MAJOR_MEDIA_TYPE_PCM 0x3
#define QMED_MAJOR_MEDIA_TYPE_MP2 0x6
#define QMED_MAJOR_MEDIA_TYPE_JPEG 0x7
#define QMED_MAJOR_MEDIA_TYPE_Q711 0x9
#define QMED_MAJOR_MEDIA_TYPE_Q728 0xa
#define QMED_MAJOR_MEDIA_TYPE_Q722 0xb
#define QMED_MAJOR_MEDIA_TYPE_Q726 0xc
#define QMED_MAJOR_MEDIA_TYPE_QOPUS 0xd
#define QMED_MAJOR_MEDIA_TYPE_MAX 0xe

#define QMED_MINOR_MEDIA_TYPE_Q711_ALAW 0x0
#define QMED_MINOR_MEDIA_TYPE_Q711_ULAW 0x1
#define QMED_MINOR_MEDIA_TYPE_Q726_ITU_BYTE_ORDER 0x0
#define QMED_MINOR_MEDIA_TYPE_Q726_IETF_BYTE_ORDER 0x1

#define QMED_VERSION (A) ((A >> 24) && 0xff)

#define QMED_SHA_SIZE            8 

typedef struct
{
    uint32_t v : 8;
    uint32_t f : 24;
} QMedFlags;

typedef struct
{
    uint32_t boxSize;
    uint32_t boxType;
    union {
        uint32_t value;
        QMedFlags     field;
    } boxFlags;
    uint32_t majorMediaType;
    uint32_t minorMediaType;
} QMedStruct;

typedef struct
{
    uint32_t hashSize; 
    uint32_t hashPayload[QMED_SHA_SIZE];
} QMedVer1InfoStruct;

typedef struct
{
    unsigned long version;
    unsigned long width;
    unsigned long height;
    unsigned long sampleTicks;
    unsigned long motionCounter;
    unsigned long motionBitmapSize;
    unsigned long qp;
} QMedH264Struct;

typedef struct
{
    uint32_t version;
    unsigned int samplingFrequency;
    unsigned int accessUnits;
    unsigned int accessUnitSize;
    unsigned int channels;
} QMedPCMStruct;

typedef struct
{
    uint32_t version;
    unsigned int samplingFrequency;
    unsigned int channels;
    unsigned int sampleSize;
    unsigned int audioSpecificConfigSize;
    // Remainder of the qmed box is the variable length audioSpecificConfig
} QMedAACStruct;

typedef struct
{
    unsigned long version;
    unsigned int samplingFrequency;
    unsigned int channels;
} QMedMP2Struct;

typedef struct
{
    unsigned long version;
    unsigned long width;
    unsigned long height;
    unsigned long frameTicks;
} QMedJPEGStruct;

typedef struct
{
    // Always: 8KHz sample rate, 64Kbps
    // Minor type in header denotes A-law or U-law.
    uint32_t version;
    unsigned int accessUnits;   // Total number of samples in box
    unsigned int sampleSize;
    //unsigned int channels;    // Always 1 channel?
} QMed711Struct;

typedef struct
{
    // Always: 1 channel, 16KHz sample rate 
    // No minor type; AMR-WB will have its own QMED type.
    uint32_t version;
    unsigned int bitrate;   // 64000, 56000, or 48000 bps for decoder; enc always 64Kbps
    unsigned int accessUnits;   // Total number of samples
    unsigned int sampleSize;
    //unsigned int channels;    // Always 1 channel?
} QMed722Struct;

typedef struct
{
    // Always: 8KHz sample rate
    uint32_t version;
    unsigned int bitrate;   // Only: 16000, 24000, 32000, or 40000
    unsigned int accessUnits;   // Total number of samples in box
    unsigned int sampleSize;
    //unsigned int channels;    // Always 1 channel?
} QMed726Struct;

typedef struct
{
    // Always: 8KHz sample rate, 16Kbps 
    uint32_t version;
    unsigned int accessUnits;   // Total number of acess units in box. Each access unit is 5 bytes long and stores 4 codewords (=20 samples).
    //unsigned int channels;    // Always 1 channel?
} QMed728Struct;

typedef struct
{
    uint32_t version;
    unsigned int samplingFrequency;
    unsigned int channels;
    unsigned int sampleSize;
    unsigned int audioSpecificConfigSize;
    unsigned int accessUnits; 
    // Remainder of the qmed box is the variable length audioSpecificConfig
} QMedOpusStruct;


#ifdef _WIN32
#define LITTLE_ENDIAN 0
#define BIG_ENDIAN    1
#define __BYTE_ORDER LITTLE_ENDIAN
#elif (defined(__APPLE__) && defined(__MACH__))
#include <machine/endian.h>
#define __BYTE_ORDER BYTE_ORDER
#else
#include <endian.h>
#endif

#define BE8(a) (a)
#if __BYTE_ORDER == BIG_ENDIAN
#define BE16(a) (a)
#define BE24(a) (a)
#define BE32(a) (a)
#define BE64(a) (a)
#else
#define BE16(a)                                                             \
    ((((a)>>8)&0xFF) |                                                      \
    (((a)<<8)&0xFF00))
#define BE24(a)                                                             \
    ((((a)>>16)&0xFF) |                                                     \
    ((a)&0xFF00) |                                                          \
    (((a)<<16)&0xFF0000))
#define BE32(a)                                                             \
    ((((a)>>24)&0xFF) |                                                     \
    (((a)>>8)&0xFF00) |                                                     \
    (((a)<<8)&0xFF0000) |                                                   \
    ((((a)<<24))&0xFF000000))
#define BE64(a)                                                             \
    (BE32(((a)>>32)&0xFFFFFFFF) |                                           \
    ((BE32((a)&0xFFFFFFFF)<<32)&0xFFFFFFFF00000000))
#endif

#ifdef __QMM__
#define QMED_MALLOC(x) QMM_MALLOC(x)
#define QMED_FREE(x)   QMM_FREE(x)
#else
#define QMED_MALLOC(x) malloc(x)
#define QMED_FREE(x)   free(x)
#endif

#endif  /* __QMED_HH */
/*******************************************************************************
* vi: set shiftwidth=4 tabstop=8 softtabstop=4 expandtab nosmarttab:
*******************************************************************************/
