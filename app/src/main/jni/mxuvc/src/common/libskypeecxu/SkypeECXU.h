// Copyright 2011-2012 Skype
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


/*
* Common definitions for Skype UVC camera library components
*/

#ifndef __SKYPE_ECXU_H__
#define __SKYPE_ECXU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

//**************Definitions*****************
typedef int8_t   s8;
typedef uint8_t  u8;
typedef int16_t  s16;
typedef uint16_t u16;
typedef int32_t  s32;
typedef uint32_t u32;
typedef int64_t  s64;
typedef uint64_t u64;

typedef int BOOL;
#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (1)
#endif

//Stream types
typedef enum {
  SID_MAIN = 0, //Main video capture stream
  SID_PREV    , //Preview video capture stream
  NUM_STREAMS ,
  //Values outside of this range are vendor defined
} sid_id_t;

//TODO: Should CUSTOM be removed from format_id_t

//Stream formats
typedef enum {
  F_YUY2 = 0 , //Raw YUY2/YUV video
  F_NV12     , //Raw NV12 video
  F_MJPEG    , //MJPEG encoded video
  F_AVC      , //AVC/H.264 video
  F_CUSTOM   , //Stream other than basic stream types
  NUM_FORMATS,
  //Values outside of this range are vendor defined
} format_id_t;

//Container for width and height
typedef struct {
  u16 width ;
  u16 height;
} res_t;

//Encapsulates processing data for one payload
typedef struct {
  u32    sid       ; //Id of stream
  u32    s_type    ; //Type of stream
  void  *payload   ; //Pointer to video payload stream
  u64    PTS       ; //Time stamp
  u32    seq_num   ; //Sequence ID to detect dropped/lost video frames
  u32    len       ; //Byte length of video payload
  res_t  resolution; //Width and height of the payload (RAW vid only)
} payload_t;

#ifdef __cplusplus
};
#endif

#endif
