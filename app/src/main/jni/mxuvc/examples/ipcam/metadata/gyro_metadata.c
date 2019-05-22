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

#define DEBUG(args...)        //printf(args)

/*******************************************************************************
* This example code explains method to capture Gyro data as Metadata stream and 
* create H264 SEI messages. Example captures one H264 stream and Metadata stream
* SEI message/NALU is written along with rest of the H264 NALU to 
* 'MXUVC_CAPTURE_FILE'
*
* Final H264 file shall look like following:
* [NAL-Delimiter] [NAL-SPS] [NAL-SEI] ... [NAL-SEI] [NAL-PPS] [NAL-SEI] ... 
* [NAL-SEI] [NAL-IDR] [NAL-Delimiter][NAL-SEI] ... [NAL-SEI] [NAL-NONIDR] 
* [NAL-Delimiter] [NAL-SEI] ... [NAL-SEI] [NAL-NONIDR] [NAL-Delimiter]

* Gyro Data is fetched from GEO Camera using independent stream/channel. Using 
* JSON configuration// we can declare one of the channel as Metadata channel. 
* Which using Mxuvc API can be started and captured. Sample JSON can be found 
* under same directory as this example code.

* Metadata Stream buffer will look like following
* [byte-0] = MXUVC_METADATA_GYRO
* [byte-1] = RESERVED
* [byte-2...] = eis_sei_msg_st

* Definition of SEI Message structure. Sensor_st will be payload of SEI message.
* resX fields are resevered data and will be filled by 0xFFFF to avoid matching 
* with NALU start sequence.
* MSByte of sensor data is 0xFF is 0xFF padded.
*******************************************************************************/

#define MXUVC_METADATA_GYRO                 1
#define MXUVC_H264_START_CODE               0x000001
#define MXUVC_NALU_SEI                      6
#define MXUVC_NALU_NONIDR                   1
#define MXUVC_NALU_IDR                      5
#define MXUVC_NALU_SPS                      7
#define MXUVC_NALU_PPS                      8
#define MXUVC_SEI_MSG_PRIV_TYPE             5

// Single metadata frame can have 42 entries
#define SEI_MAX_MSG_ENTRY                   42
#define SEI_MAX_Q_ENTRY                     10

#define MXUVC_CAPTURE_FILE                  "/tmp/out.h264"

#define PAYLOAD_SIZE(cnt)   (                                \
    sizeof(struct eis_sei_msg_st) -                          \
    ( (SEI_MAX_MSG_ENTRY - cnt) * sizeof(struct sensor_st) ) \
    )

struct sensor_st {
    unsigned short gyro_x;            // calibrated data
    unsigned short reserved1;
    unsigned short gyro_y;            // calibrated data
    unsigned short reserved2;
    unsigned short gyro_z;            // calibrated data
    unsigned short reserved3;
    unsigned short accel_x;           // calibrated data
    unsigned short reserved4;
    unsigned short accel_y;           // calibrated data
    unsigned short reserved5;
    unsigned short accel_z;           // calibrated data
    unsigned short reserved6;
};

struct eis_sei_msg_st {
    unsigned int sensor;              // sensor data mask
    unsigned int num;      
    struct sensor_st st[SEI_MAX_MSG_ENTRY];
};

struct gyro_frame {
    unsigned long long timestamp;
    struct eis_sei_msg_st eis;
};

/* 
 In actual implementation, the following UUID should change according to 
 metadataType to distinguish one type from another 
 e4a6b160-916e-11d9-bfdc-0002b3623fd1
*/
const char quuid[16] =
    { 0xe4, 0xa6, 0xb1, 0x60, 0x91, 0x6e, 0x11, 0xd9, 0xbf, 0xdc, 0x00, 0x02,
      0xb3, 0x62, 0x3f, 0xd1
    };

const char delimiter_nal[6] = { 0x0, 0x0, 0x0, 0x1, 0x9, 0x30 };
const char start_code[4] = { 0x0, 0x0, 0x0, 0x1 };

static FILE *fd = NULL;

static struct gyro_frame sei_queue[SEI_MAX_Q_ENTRY];
static int qrd = 0, qwr = 0;

/*******************************************************************************
* ch_cb() is called for every frame received from GEO Camera.
* This callback funciton is registered for H264 as well as for Metadata channel 
* stream. video_info_t strucutre provides format of frame and timestamp (33 bit,
*  90KHz clock). Callback checks for format of stream. 

* If format is Metadata type, SEI message queue is updated with new frame 
* contents. For each struct sensor_st received in, a SEI message  is created == 
* entry in sei_queue. 

* If Format is H264 type, frame is parsed for checking NAL type and start index. 
* If NAL is SPS, PPS, IDR, NONIDR type SEI message is written to file based on 
* timestamp comparision. All SEI message which has smaller timestamp value will 
* be written before new H264 NAL. 

* A queue model between h264 call back and writter thread shall be implemented 
* if more processing/parsing of h264 or SEI is required. Processing can delay 
* return of call back. Delay in returning call back can lead to buffer starving 
* at v4l2 layer. Resulting in frames drops OR streaming stop case
*******************************************************************************/

static void ch_cb(unsigned char *buffer, unsigned int size,
          video_info_t info, void *user_data)
{
    struct gyro_frame * gframe = NULL;
    struct eis_sei_msg_st * eis = NULL;
    int i = 0, k = 0;
    
    video_format_t fmt = (video_format_t) info.format;
    video_channel_t ch = (video_channel_t) user_data;

    if (fmt < FIRST_VID_FORMAT || fmt >= NUM_VID_FORMAT) {
        printf("Unknown Video format\n");
        return;
    }

    if (fd == NULL) {
        fd = fopen(MXUVC_CAPTURE_FILE, "w");
        if (fd == NULL) {
            printf("Unable to open file: %s\n", MXUVC_CAPTURE_FILE);
        }
    }

    if (fmt == VID_FORMAT_METADATAS) {
        eis = (struct eis_sei_msg_st *) (buffer + 2);

        DEBUG("METADATAS: ts %llu size %d \n", info.ts, size);

        if (buffer[0] != MXUVC_METADATA_GYRO
            && (size <= sizeof(struct sensor_st))) {
            printf("Metadata type %d, size %d\n", (unsigned int)buffer[0], 
                    size);
            goto end;
        }
        
        if ((eis->num & 0xFF) <= 0) {
            printf("Only header in metadata frame Cnt %d\n", eis->num & 0xFF);
            goto end;
        }

        DEBUG("Count %d\n", eis->num & 0xFF);

        // Update queue payload = 'struct sensor_st'
        sei_queue[qwr].timestamp = info.ts;
        memcpy(&sei_queue[qwr].eis, eis, PAYLOAD_SIZE(eis->num & 0xFF));
        
        qwr++;
        if (qwr >= SEI_MAX_Q_ENTRY)
            qwr = 0;

        DEBUG("qwr:%d qrd:%d ts:%llu\n", qwr - 1, qrd, 
            sei_queue[qwr - 1].timestamp);
    }

    if (fmt == VID_FORMAT_H264_RAW) {
        unsigned int val = 0xffffffff;
        unsigned int offset = 0;
        unsigned char nal_type = 0;
        unsigned char buf[128];
        unsigned char ch = 0x80;
        bool update_sei = false;
        unsigned char *ptr = buffer;
        
        DEBUG("H264: ts %llu\n", info.ts);

        while (offset < size - 3) {
            
            val <<= 8;
            val |= *ptr++;
            offset++;
            
            if (val == MXUVC_H264_START_CODE) {
                
                nal_type = *ptr & 0x1f;

                if (nal_type == MXUVC_NALU_SPS
                    || nal_type == MXUVC_NALU_PPS
                    || nal_type == MXUVC_NALU_IDR
                    || nal_type == MXUVC_NALU_NONIDR) {
                    
                    DEBUG("found NAL %d offset %d \n", nal_type, offset - 4);
                    
                    update_sei = true;
                    break;
                }
            }
        }

        // Write SEI Messages from sei_queue    
        if (update_sei == true) {
    
            //write NALs before desired-NAL in the buffer
            fwrite(buffer, offset - 4, 1, fd);

            gframe = &sei_queue[qrd];
            eis = &gframe->eis;
            
            DEBUG("Timestamps %llu <= %llu QRD %d QWR %d\n", 
                gframe->timestamp, info.ts, qrd, qwr);

            while (gframe->timestamp && (gframe->timestamp <= info.ts)) {

                DEBUG("Timestamps %llu <= %llu\n", gframe->timestamp, info.ts);

                ptr = buf;
                memcpy(ptr, start_code, 4);
                ptr += 4;
                *ptr++ = MXUVC_NALU_SEI;
                *ptr++ = MXUVC_SEI_MSG_PRIV_TYPE;
                
                k = (PAYLOAD_SIZE(eis->num & 0xFF) + 16 ) / 255;
                for (i = 0; i < k; i++)
                    *ptr++ = 0xff;
               *ptr++ = (PAYLOAD_SIZE(eis->num & 0xFF) + 16 ) % 255;
                
                // Write uuid (16 bytes)
                for (k = 0; k < 16; k++)
                    *ptr++ = quuid[k];
                
                fwrite(buf, ptr - buf, 1, fd);

                fwrite(eis, PAYLOAD_SIZE(eis->num & 0xFF), 1, fd);
                fwrite(&ch, sizeof(unsigned char), 1, fd);
                        
                memset(&sei_queue[qrd], 0, sizeof(struct eis_sei_msg_st));
                
                qrd++;
                if (qrd >= SEI_MAX_Q_ENTRY)
                    qrd = 0;

                eis = &sei_queue[qrd].eis;
            }

            val = offset - 4;

            fwrite(&buffer[offset - 4], size - val, 1, fd);
        } else {
            fwrite(buffer, 1, size, fd);
        }
    }

end:
    mxuvc_video_cb_buf_done(ch, info.buf_index);
}

int main(int argc, char **argv)
{
    int ret = 0;
    int ch_count = 0;
    long channel;
    camer_mode_t mode;
    int i;

    ret = mxuvc_video_init("v4l2", "dev_offset=0");

    if (ret < 0)
        return 1;

    ret = mxuvc_video_register_cb(CH1, ch_cb, (void *)CH1);
    if (ret < 0)
        goto error;

    ret = mxuvc_video_get_channel_count(&ch_count);

    printf("Total Channel count: %d\n", ch_count);

    // Point to metadata channel in the count
    ch_count = ch_count - 2;

    ret = mxuvc_video_register_cb(ch_count, ch_cb, (void *)ch_count);
    if (ret < 0)
        goto error;

    ret = mxuvc_video_start(CH1);
    if (ret < 0)
        goto error;

    ret = mxuvc_video_start(CH4);
    if (ret < 0)
        goto error;

    usleep(5000);

    int counter;
    if (argc > 1) {
        counter = atoi(argv[1]);
    } else
        counter = 15;

    while (counter--) {
        if (!mxuvc_video_alive()) {
            goto error;
        }
        printf("\r%i secs left\n", counter + 1);
        fflush(stdout);
        sleep(1);
    }

    ret = mxuvc_video_stop(CH1);
    if (ret < 0)
        goto error;

    ret = mxuvc_video_stop(CH4);
    if (ret < 0)
        goto error;

    mxuvc_video_deinit();
    
    fclose(fd);

    printf("Success\n");

    return 0;
error:
    mxuvc_video_deinit();
    fclose(fd);
    printf("Failure\n");
    return 1;
}
