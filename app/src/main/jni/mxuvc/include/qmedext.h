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
* @(#) $Id: qmedext.h 60829 2017-03-09 21:44:30Z nmasood $
*******************************************************************************/

#ifndef __QMEDEXT_HH
#define __QMEDEXT_HH

//Following are bit-wise
#define QMED_FLAGS_DEWARP          0x01
#define QMED_FLAGS_SENSOR          0x02
#define QMED_FLAGS_AWB             0x04
#define QMED_FLAGS_AWBINPUTSTATS   0x08
#define QMED_FLAGS_FULLAEHISTOGRAM 0x10

//Types
typedef enum {
    QMED_EXT_TYPE_DEWARP          = 0x01,
    QMED_EXT_TYPE_SENSOR          = 0x02,
    QMED_EXT_TYPE_AWB             = 0x03,
    QMED_EXT_TYPE_AWBINPUTSTATS   = 0x04,
    QMED_EXT_TYPE_FULLAEHISTOGRAM = 0x05
} qmed_ext_type_t;

#ifdef _WIN32
#define LITTLE_ENDIAN 0
#define BIG_ENDIAN    1
#define __BYTE_ORDER LITTLE_ENDIAN
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

//Each extension consists of following header followed by specific data
typedef struct
{
    unsigned long boxSize;
    unsigned long boxType;
} QMedExtStruct;

//Following structures contain values in native endianness of codec
typedef struct
{
    QMedExtStruct hdr;
    int mapType;     //Of type EPTZ_MAP_TYPES as defined in codec/lib/dewarp/ePTZ_eWARP.h
    short divisor;
    short panel;
    int params[22];
} QMedExtDewarpStruct;

typedef struct
{
    QMedExtStruct hdr;
    // VTS: vertical total size in number of lines.
    unsigned int vts;
    // Row time is the time taken for one row to be "clocked" or operated upon,
    // inside the sensor. It is the time taken in microseconds for that
    // operation to move from the first pixel of one row to the first pixel of
    // the next row.
    // Integration rows is then the number of row times for which a pixel
    // integrates. It is the duration between the reset and readout of a pixel.
    // The minimum and maximum integration time can be set in the JSON file with
    // parameters "ae.min_exposure" and "ae.max_exposure".
    unsigned int integrationRows;
    // Sensor total gain in Q24.8 format. The maximum gain is set in the JSON
    // file using "ae.maxgain".
    unsigned int totalGain_q24_8;
    // Exposure value calculated by the AE for the scene in Q24.8 format. AE
    // then decides how to partition this EV between sensor total gain and
    // integration rows.
    // Formula:
    // unclippedEV = (totalGain * integrationRows)
    // - totalGain: current sensor total gain.
    // - integrationRows: current sensor integration rows.
    // NOTE: "unclippedEV" is the unclipped exposure value calculated by the AE
    // without taking into account maximum gain or integration rows. Therefore,
    // when "totalGain" is at maximum value, "unclippedEV" values may not match
    // "(totalGain * integrationRows)" since AE will clip total gain.
    unsigned int unclippedEV_q24_8;
    // Integration time (in ms) in Q24.8 format.
    // Formula:
    //                              1
    // integrationTime (ms) = ----------- * integrationRows * 1000
    //                         (fps * VTS)
    unsigned int integrationTime_q24_8;
} QMedExtSensorStruct;

typedef struct
{
    QMedExtStruct hdr;
    // Red, green and blue gains applied to white balance the image. Format is
    // unsigned 4.8 fixed point. Lower limit is always 1.0 (0x100) and the
    // default upper limit is 4.5 (0x480). The upper limit can be changed in the
    // JSON file using the parameter "final_gain_clamp".
    unsigned int awbFinalRG;
    unsigned int awbFinalGG;
    unsigned int awbFinalBG;
    unsigned int awbRedGreenRatio;
    unsigned int awbBlueGreenRatio;
    // Correlated color temperature of the scene illumination in Kelvin. Typical
    // range is between 2500K (warm illuminants) to 6500K (cool illuminants).
    unsigned int awbColorTemperature;
} QMedExtAwbStruct;

#ifndef __ISPLIB_H      //To prevent redeclaration
typedef struct
{
    // Average R/G ratio for individual zone. Format is unsigned 4.8 fixed-
    // point.
    unsigned short    rg;
    // Average B/G ratio for individual zone. Format is unsigned 4.8 fixed-
    // point.
    unsigned short    bg;
    // Number of pixels used in AWB statistics for individual zone. Format is 
    // integer.
    unsigned int      sum;
} STRUCT_AWB_STATS;
#endif

#define ISP_METERING_ZONES_X            15
#define ISP_METERING_ZONES_Y            15
#define ISP_NUM_METERING_ZONES          (ISP_METERING_ZONES_X * ISP_METERING_ZONES_Y)

typedef struct
{
    QMedExtStruct hdr;
    // AWB statistics for each zone: R/G ratio, B/G ratio, and sum (total number
    // pixels used in AWB statistics). Color ratios are in unsigned 4.8 fixed-
    // point format and total number of pixels is in integer format.
    // Formula for R/G ratio is:
    //                          R_p
    // RG_mn =      sum        ----- * Theta_p
    //         (p in Omega_mn)  G_p
    // where:
    // - RG_mn: R/G ratio for individual zone (m x n)
    // -      sum       : sum of pixels that belong to individual zone Omega of
    //  (p in Omega_mn)   size (m x n).
    // - R_p: red pixels that belong to individual zone Omega.
    // - G_p: green pixels that belong to individual zone Omega.
    // - Theta_p: white locus indicator. Equal to 1 if pixel is likely to belong
    //   to a white area, as defined by the region of color-spaced bounded by
    //   the settings max_rg, max_bg, and min_rg and min_bg.
    // NOTE: awb.white_level and awb.black_level set the upper and lower
    // intensity boundaries within which pixels contribute to the statistics.
    STRUCT_AWB_STATS    awbZoneStats[ISP_NUM_METERING_ZONES];
} QMedExtAwbInputStatsStruct;

#define FULL_HISTOGRAM_BANDS    256

typedef struct
{
    QMedExtStruct hdr;
    // Full 256-bin histogram (normalized). Each histogram bin is pixel count in
    // integer format. Histogram bins are weighted by zone.
    // Total histogram bin counts should be less than or equal to width*height/4
    // (since only Gr of the Bayer patterns is used).
    int fullHistogram[FULL_HISTOGRAM_BANDS];
} QMedExtFullAEHistogramStruct;

/* Metadata macro (for host) */
#define METADATA_GET(metadata, data_ptr, data_struct_type, qmed_type) \
    do { \
        unsigned int cur_offset = 0; \
        data_ptr = NULL; \
        while(cur_offset < metadata->total_len) { \
            QMedExtStruct *extension = \
                (QMedExtStruct*)(metadata->qmedExt+cur_offset); \
            unsigned long size = BE32(extension->boxSize); \
            unsigned long type = BE32(extension->boxType); \
            cur_offset += size; \
            \
            if(type != qmed_type) { \
                continue; \
            } else { \
                data_ptr = (data_struct_type*) extension; \
                break; \
            } \
        } \
    } while(0)

#endif  /* __QMEDEXT_HH */
/*******************************************************************************
* vi: set shiftwidth=4 tabstop=8 softtabstop=4 expandtab nosmarttab:
*******************************************************************************/
