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
#include <mxuvc.h>
#include "common.h"
#include "qbox.h"
#include "qmedutil.h"
#include "qmed.h"
#include "qmedext.h"
#include "qboxutil.h"

#define QBOX_TYPE   (('q' << 24) | ('b' << 16) | ('o' << 8) | ('x'))

#if __BYTE_ORDER == LITTLE_ENDIAN
#define be32_to_cpu(x)  ((((x) >> 24) & 0xff) | \
             (((x) >> 8) & 0xff00) | \
             (((x) << 8) & 0xff0000) | \
             (((x) << 24) & 0xff000000))
#define be24_to_cpu(a)   \
    ((((a)>>16)&0xFF) |                                                     \
    ((a)&0xFF00) |                                                          \
    (((a)<<16)&0xFF0000))
#define be16_to_cpu(x)  ((((x) >> 8) & 0xff) | \
             (((x) << 8) & 0xff00))
#else
#define be32_to_cpu(x) (x)
#define be24_to_cpu(x) (x)
#define be16_to_cpu(x) (x)
#endif

//QBOX sample type is a 16 bit field
#define QBOX_SAMPLE_TYPE_AAC 0x1
#define QBOX_SAMPLE_TYPE_QAC 0x1
#define QBOX_SAMPLE_TYPE_H264 0x2
#define QBOX_SAMPLE_TYPE_QPCM 0x3
#define QBOX_SAMPLE_TYPE_DEBUG 0x4
#define QBOX_SAMPLE_TYPE_H264_SLICE 0x5
#define QBOX_SAMPLE_TYPE_QMA 0x6
#define QBOX_SAMPLE_TYPE_VIN_STATS_GLOBAL 0x7
#define QBOX_SAMPLE_TYPE_VIN_STATS_MB 0x8
#define QBOX_SAMPLE_TYPE_Q711 0x9
#define QBOX_SAMPLE_TYPE_Q722 0xa
#define QBOX_SAMPLE_TYPE_Q726 0xb
#define QBOX_SAMPLE_TYPE_Q728 0xc
#define QBOX_SAMPLE_TYPE_JPEG 0xd
#define QBOX_SAMPLE_TYPE_MPEG2_ELEMENTARY 0xe
#define QBOX_SAMPLE_TYPE_USER_METADATA 0xf
#define QBOX_SAMPLE_TYPE_STAT_LUMA     0x16   //Upper byte contains the analytics combination
#define QBOX_SAMPLE_TYPE_STAT_NV12     0x17   //Upper byte contains the analytics combination
#define QBOX_SAMPLE_TYPE_QOPUS 0x18
#define QBOX_SAMPLE_TYPE_METADATAS 0x23

//#define QBOX_SAMPLE_FLAGS_CONFIGURATION_INFO 0x0001
#define QBOX_SAMPLE_FLAGS_CTS_PRESENT        0x0002
//#define QBOX_SAMPLE_FLAGS_SYNC_POINT         0x0004
#define QBOX_SAMPLE_FLAGS_DISPOSABLE         0x0008
#define QBOX_SAMPLE_FLAGS_MUTE               0x0010
#define QBOX_SAMPLE_FLAGS_BASE_CTS_INCREMENT 0x0020
#define QBOX_SAMPLE_FLAGS_META_INFO          0x0040
#define QBOX_SAMPLE_FLAGS_END_OF_SEQUENCE    0x0080
#define QBOX_SAMPLE_FLAGS_END_OF_STREAM      0x0100
#define QBOX_SAMPLE_FLAGS_QMED_PRESENT       0x0200
#define QBOX_SAMPLE_FLAGS_PKT_HEADER_LOSS    0x0400
#define QBOX_SAMPLE_FLAGS_PKT_LOSS           0x0800
#define QBOX_SAMPLE_FLAGS_120HZ_CLOCK        0x1000
#define QBOX_SAMPLE_FLAGS_TS                 0x2000
#define QBOX_SAMPLE_FLAGS_TS_FRAME_START     0x4000
#define QBOX_SAMPLE_FLAGS_PADDING_MASK       0xFF000000

#define QBOX_VERSION(box_flags)                                         \
    ((box_flags) >> 24)
#define QBOX_BOXFLAGS(box_flags)                                        \
    (((box_flags) << 8) >> 8)
#define QBOX_FLAGS(v, f)                                                \
    (((v) << 24) | (f))
#define QBOX_SAMPLE_PADDING(sample_flags)                               \
    (((sample_flags) & QBOX_SAMPLE_FLAGS_PADDING_MASK) >> 24)
#define QBOX_SAMPLE_FLAGS_PADDING(sample_flags, padding)                \
    ((sample_flags) | ((padding) << 24))

//qmed functions
#define GetExtPtr(pHdr)  \
    ((GetQMedBaseVersion(pHdr) == 1) ? (pHdr + sizeof(QMedStruct) + sizeof(QMedVer1InfoStruct)) : \
                                      (pHdr + sizeof(QMedStruct)))
    
typedef struct
{
    uint32_t v : 8;
    uint32_t f : 24;
} QBoxFlag;

typedef struct
{
    uint32_t samplerate;
    uint32_t samplesize;
    uint32_t channels;
} QBoxMetaA;

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t gop;
    uint32_t frameticks;
} QBoxMetaV;

typedef union
{
    QBoxMetaA a;
    QBoxMetaV v;
} QBoxMeta;

typedef struct
{
    uint32_t addr;
    uint32_t size;
} QBoxSample;

//////////////////////////////////////////////////////////////////////////////
// version: 1
//
// 64 bits cts support

typedef struct
{
    uint32_t ctslow;
    uint32_t addr;
    uint32_t size;
} QBoxSample1;


struct qbox_sample {
    uint32_t addr;
    uint32_t size;
};

struct qbox_sample_v1 {
    uint32_t ctslow;
    uint32_t addr;
    uint32_t size;
};

typedef struct qbox_header {
    uint32_t box_size;
    uint32_t box_type;
    /* TODO test on BE and LE machines */
    struct {
        uint32_t version : 8;
        uint32_t flags   : 24;
    } box_flags;
    uint16_t stream_type;
    uint16_t stream_id;
    uint32_t sample_flags;
    uint32_t sample_cts;
    uint32_t sample_cts_low;
}__attribute__((__packed__)) qbox_header_t;


/* QBOX header parser
 * buf: data buffer passed by application
 * channel_id: channel id
 * fmt: data format of the channel
 * data_buf: data starts here
 * size: size of data
 * ts: timestamp; only 32 bit timestamps are passed to application
 * Returns 0 if QBOX header was parsed successfully
 * Returns 1 if the frame is not a valid qbox frame
 */
int qbox_parse_header(uint8_t *buf, int *channel_id, video_format_t *fmt,
                      uint8_t **data_buf, uint32_t *size,
                      uint64_t *ts, uint32_t *analytics,
                      metadata_t *metadata,
                      qmed_t *qmed)
{
    unsigned char *p = buf;
    qbox_header_t *qbox = (qbox_header_t*)buf;
    int hdr_len;
    int qmed_len=0;
    uint32_t flags;
    uint16_t streamType;

    if (be32_to_cpu(qbox->box_type) != QBOX_TYPE) {
        return 1;
    }

    //Initialize to no QMED
    qmed->total_len = 0;
    qmed->qmedPtr = NULL;
    hdr_len = sizeof(qbox_header_t);
    if (qbox->box_flags.version == 1) {
        *ts = ((uint64_t)be32_to_cpu(qbox->sample_cts) << 32) | be32_to_cpu(qbox->sample_cts_low);
    } else {
        *ts = be32_to_cpu(qbox->sample_cts);
        hdr_len -= sizeof(uint32_t);
    }

    //map qbox header type to video_format_t type before passing to user
    streamType = be16_to_cpu(qbox->stream_type);
    switch (streamType) {
    case QBOX_SAMPLE_TYPE_AAC:
        *fmt = VID_FORMAT_H264_AAC_TS;
        break;
    case QBOX_SAMPLE_TYPE_QOPUS:
        *fmt = VID_FORMAT_H264_AAC_TS;
        break;
    case QBOX_SAMPLE_TYPE_H264:
        flags = be32_to_cpu(qbox->sample_flags);
        if (0 != (flags & QBOX_SAMPLE_FLAGS_TS)) {
            *fmt = VID_FORMAT_H264_TS;
        } else {
            *fmt = VID_FORMAT_H264_RAW;
            if (be32_to_cpu(qbox->sample_flags) & QBOX_SAMPLE_FLAGS_QMED_PRESENT) {
                QMedStruct *pQMed;
                int qmedVersion;
                // skip the qbox header
                p += hdr_len;
                qmedVersion = GetQMedBaseVersion(p);
                pQMed = (QMedStruct *) p;
                qmed_len = be32_to_cpu(pQMed->boxSize);
                qmed->qmedPtr = (char *)pQMed;
                qmed->total_len = qmed_len;
                p = GetExtPtr(p) + sizeof(QMedH264Struct);
                metadata->qmedExt = (char *) p;
                metadata->total_len = qmed_len
                    - sizeof(QMedStruct)
                    - (qmedVersion == 1 ? sizeof(QMedVer1InfoStruct) : 0)
                    - sizeof(QMedH264Struct);
            }
        }
        break;
    case QBOX_SAMPLE_TYPE_JPEG:
        *fmt = VID_FORMAT_MJPEG_RAW;
        if (be32_to_cpu(qbox->sample_flags) & QBOX_SAMPLE_FLAGS_QMED_PRESENT) {
            QMedStruct *pQMed;
            int qmedVersion;
            // skip the qbox header
            p += hdr_len;
            qmedVersion = GetQMedBaseVersion(p);
            pQMed = (QMedStruct *) p;
            qmed_len = be32_to_cpu(pQMed->boxSize);
            qmed->qmedPtr = (char *)pQMed;
            qmed->total_len = qmed_len;
            p = GetExtPtr(p) + sizeof(QMedJPEGStruct);
            metadata->qmedExt = (char *) p;
            metadata->total_len = qmed_len
                - sizeof(QMedStruct)
                - (qmedVersion == 1 ? sizeof(QMedVer1InfoStruct) : 0)
                - sizeof(QMedJPEGStruct);
        }
        break;
    case QBOX_SAMPLE_TYPE_METADATAS:
        *fmt = VID_FORMAT_METADATAS;
        break;

    default:
        if((streamType & 0xff) == QBOX_SAMPLE_TYPE_STAT_LUMA)
        {
            *fmt = VID_FORMAT_GREY_RAW;
            *analytics = streamType >> 8;
        }
        else if((streamType & 0xff) == QBOX_SAMPLE_TYPE_STAT_NV12)
        {
            *fmt = VID_FORMAT_NV12_RAW;
            *analytics = streamType >> 8;
        }
        else
        {
            TRACE("Wrong Qbox format\n");
        }
        break;
    }
    *channel_id = be16_to_cpu(qbox->stream_id);
    //*fmt = be16_to_cpu(qbox->stream_type);
    *data_buf = ((uint8_t*)buf + hdr_len + qmed_len);
    *size = be32_to_cpu(qbox->box_size) - hdr_len - qmed_len;

    return 0;
}



static const int SampleFreq[12] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000
};

unsigned int GetQMedAACAudioSpecificConfigSize(unsigned char *pQMed)
{
    QMedAACStruct *pQMedAAC = (QMedAACStruct *) GetExtPtr(pQMed);

    return BE32(pQMedAAC->audioSpecificConfigSize);
}

uint32_t GetQMedBaseVersion(unsigned char *pQMed)
{
    QMedStruct *pQMedBase = (QMedStruct *) pQMed;

    return BE8(pQMedBase->boxFlags.field.v);
}

unsigned long GetQMedH264QP(unsigned char *pQMed)
{
    QMedH264Struct *pQMedH264 = (QMedH264Struct *) GetExtPtr(pQMed);

    return BE32(pQMedH264->qp);
}

uint32_t GetQMedAACHeaderSize(int baseVersion, int aacVersion)
{
    if (baseVersion == 0)
        return (sizeof(QMedStruct) + sizeof(QMedAACStruct));
    else
        return (sizeof(QMedStruct) + sizeof(QMedVer1InfoStruct) + sizeof(QMedAACStruct));
}

unsigned int GetQMedAACSampleFrequency(unsigned char *pQMed)
{
    QMedAACStruct *pQMedAAC = (QMedAACStruct *) GetExtPtr(pQMed);

    return BE32(pQMedAAC->samplingFrequency);
}

unsigned int GetQMedAACChannels(unsigned char *pQMed)
{
    QMedAACStruct *pQMedAAC = (QMedAACStruct *) GetExtPtr(pQMed);

    return BE32(pQMedAAC->channels);
}

uint32_t GetQMedBaseBoxSize(unsigned char *pQMed)
{
    QMedStruct *pQMedBase = (QMedStruct *) pQMed;

    return BE32(pQMedBase->boxSize);
}

uint32_t GetQMedOpusHeaderSize(int baseVersion, int aacVersion)
{
    if (baseVersion == 0)
        return (sizeof(QMedStruct) + sizeof(QMedOpusStruct));
    else
        return (sizeof(QMedStruct) + sizeof(QMedVer1InfoStruct) + sizeof(QMedOpusStruct));
}

uint32_t GetQmedOpusVersion(unsigned char *pQMed)
{
    QMedOpusStruct *pQMedOpus = (QMedOpusStruct *) GetExtPtr(pQMed);

    return BE32(pQMedOpus->version);
}

unsigned int  GetQmedOpusAccessUnits(unsigned char *pQMed)
{
    QMedOpusStruct *pQMedOpus = (QMedOpusStruct *) GetExtPtr(pQMed);

    return BE32(pQMedOpus->accessUnits);
}

unsigned int  GetQmedOpusSampleSize(unsigned char *pQMed)
{
    QMedOpusStruct *pQMedOpus = (QMedOpusStruct *) GetExtPtr(pQMed);

    return BE32(pQMedOpus->sampleSize);
}

unsigned int GetQMedOpusSampleFrequency(unsigned char *pQMed)
{
	QMedOpusStruct *pQMedOpus = (QMedOpusStruct *) GetExtPtr(pQMed);

    return BE32(pQMedOpus->samplingFrequency);
}

unsigned int GetQMedOpusChannels(unsigned char *pQMed)
{
	QMedOpusStruct *pQMedOpus = (QMedOpusStruct *) GetExtPtr(pQMed);

    return BE32(pQMedOpus->channels);
}

int get_qbox_hdr_size(void)
{
    return sizeof(qbox_header_t);
}

int audio_param_parser(audio_params_t *h, unsigned char *buf, int len)
{
    unsigned char *p = buf;
    // params for audiospecific config
    unsigned char *asc;
    int ascSize;
    unsigned char *pQMedAAC = NULL;
    unsigned char *pQMedOpus = NULL;
    int hdr_len;

    qbox_header_t *qbox = (qbox_header_t *)buf;

    if (be32_to_cpu(qbox->box_type) != QBOX_TYPE) {
        TRACE("ERR: Wrong mux format\n");
        return 1;
    }

    hdr_len = sizeof(qbox_header_t);

    if (qbox->box_flags.version == 1) {
        h->timestamp = be32_to_cpu(qbox->sample_cts_low);
    } else {
        h->timestamp = be32_to_cpu(qbox->sample_cts);
        hdr_len -= sizeof(uint32_t);
    }

    if (be16_to_cpu(qbox->stream_type) == QBOX_SAMPLE_TYPE_AAC)
    {

        // skip the qbox header
        p += hdr_len;
        len -= hdr_len;

        // get the media description box to build the adts header
        if (be32_to_cpu(qbox->sample_flags) & QBOX_SAMPLE_FLAGS_QMED_PRESENT)
        {
            pQMedAAC = (unsigned char *) p;

            // retrieve the audio specific config size
            ascSize = GetQMedAACAudioSpecificConfigSize(pQMedAAC);

            if (!ascSize)
            {
                TRACE("ERROR !!! No ASC found\n");
                return 1;
            }

            asc = (unsigned char *) (p + GetQMedAACHeaderSize(GetQMedBaseVersion(pQMedAAC), 0));


            h->samplefreq = GetQMedAACSampleFrequency(pQMedAAC);
            h->channelno = GetQMedAACChannels(pQMedAAC);
            // skip past the media box
            p += GetQMedBaseBoxSize(pQMedAAC);
            len -= GetQMedBaseBoxSize(pQMedAAC);


            h->audioobjtype = asc[0] >> 3;

            // support 1 or 2 ch only
            if ((h->channelno < 1) ||
                    (h->channelno > 2))
            {
                TRACE("Channel number unsupported %d", h->channelno);
            }

        }

        h->dataptr = p;
        h->framesize = len;
        h->format = AUD_FORMAT_AAC_RAW;
    }
    else if (be16_to_cpu(qbox->stream_type) == QBOX_SAMPLE_TYPE_QOPUS)
    {

        // skip the qbox header
        p += hdr_len;
        len -= hdr_len;

        // get the media description box to build the adts header
        if (be32_to_cpu(qbox->sample_flags) & QBOX_SAMPLE_FLAGS_QMED_PRESENT)
        {
            pQMedOpus = (unsigned char *) p;

            // skip past the media box
            p += GetQMedBaseBoxSize(pQMedOpus);
            len -= GetQMedBaseBoxSize(pQMedOpus);

/*
            // support 1 or 2 ch only
            if ((h->channelno < 1) ||
                    (h->channelno > 2))
            {
                TRACE("Channel number unsupported %d", h->channelno);
            }
*/
            h->samplefreq = GetQMedOpusSampleFrequency(pQMedOpus);
            h->channelno = GetQMedOpusChannels(pQMedOpus);
        }

        h->dataptr = p;
        h->framesize = len;
        h->format = AUD_FORMAT_OPUS_RAW;
    }

    else
    {
        TRACE("Unsupported Audio Codec \n");
        return 1;
    }


    return 0;
}
