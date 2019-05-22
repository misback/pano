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


/**********************************************************************
* FILE   : skypecamintfc.c
* PURPOSE: Implements parser for Skype UVC XU frames
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "skypeecxuparser.h"

//***********Defines:
//#define TEST_STREAM_SIZE (64*1024) //64k memory size
#define MAGIC_GUID 0x534B5950 //SKYP

//Debug printing macro
#ifdef DBG_PRINT
#define debug_printf(...) \
   do{ printf("PARSER->%d: ", __LINE__); printf(__VA_ARGS__); }while (0)
#else
#define debug_printf(...)
#endif

#define PTS_BIT_LEN   64
#define SID_BIT_LEN    8
#define STYPE_BIT_LEN  8
#define SQNUM_BIT_LEN 16
#define POSET_BIT_LEN 32
#define PSIZE_BIT_LEN 32
//Maximum number of payloads possible in a single frame
#define MAX_PLDS_PER_FRM (1 << STYPE_BIT_LEN)

//***********Data Structures:

//This structure holds the data pointer and meta data for ALL CURRENT payloads.
//It's size is limited by the maximum number of stream types allowed in a single
//frame.  Even if there are dual endpoints and both endpoints use the Skype 
//Transport Stream Format, the StreamIDs sent by each interface must be
//mutually exclusive.  This equals 2^8.
struct frame_s {
  payload_t parsed_payloads[MAX_PLDS_PER_FRM]; //Payload data
  BOOL      valid          [MAX_PLDS_PER_FRM]; //Validity data
  u32       cur_payload_cnt                  ; //Number of current payloads
};

//Internal storage for parsed frame data
frame_t cur_parsed_frame;

#pragma pack(push,1) //Enforce single byte alignment
typedef struct {
  u64 PTS           : PTS_BIT_LEN  ; //64 bit timestamp
  u8  StreamID      : SID_BIT_LEN  ; //ID of stream
  u8  StreamType    : STYPE_BIT_LEN; //Stream type
  u16 SequenceNum   : SQNUM_BIT_LEN; //Frame's sequence number
  u32 PayloadOffset : POSET_BIT_LEN; //Byte offset to this streams video in UVC frame
  u32 PayloadSize   : PSIZE_BIT_LEN; //Size in bytes of the stream's video
} skype_xu_hdr_t;
#pragma pack(pop)

//***********Constants:

static const u32 XU_HDR_SZ = sizeof( skype_xu_hdr_t );

//The following string array provide human readable strings for error values.
//When accessing the array, ensure a positive value is used.  Error codes
//are negative values and will obviously access invalid memory if used
//directly.  Ensure the positive version of the error code is used for array
//access.  i.e. PARSE_ERROR_STRINGS[0-error_code].  A function is provided 
//to help avoid this type of error.

//Strings for parser status enum.
const char *PARSE_ERROR_STRINGS[PARSE_ERROR_CNT] = {
 "PARSE_NO_ERROR"        ,
 "PARSE_NO_MAGIC"        ,
 "PARSE_NO_HEADER_COUNT" ,
 "PARSE_HDR_SEC_OVERFLOW",
 "PARSE_PAYLOAD_ERROR"   ,
 "PARSE_BAD_INPUT"       ,
 "PARSE_SID_COLLISION"   ,
};

//Endian indicator
static BOOL is_big_e;

//***********Local Functions:

//The following three functions are present in the standard C library; however,
//on some systems the dependencies are large.  For this reason, these functions
//are written locally to minimize large library dependencies.

//Function to check endianess of host machine.
BOOL is_big_endian_l( void ) {
  static u32  x = 0x01020304; //Set bit pattern
  //Check starting byte
  static BOOL result;
  result = (s32)(*((unsigned char *)(&x))) == 1 ? TRUE : FALSE;

  return result;
}

//32 bit network byte order to host byte order function.
u32 ntohl_l( u32 network_word ) {
  u32  ret_val   = network_word;
  u8   *src_ptr  = (u8 *)&network_word; //Point to front
  u8   *dest_ptr = ((u8 *)&ret_val) + sizeof(network_word) - 1; //Point to back
  u32  idx       = 0;

  //If not big endian, reverse byte order
  if( is_big_e == FALSE ) {
    for( ; idx < sizeof(network_word); idx++ ) {
      *dest_ptr-- = *src_ptr++; //Reverse the byte order
    } 
  } //else return word unmolested.

  return ret_val;
};

//16 bit network byte order to host byte order function.
u16 ntohs_l( u16 network_word ) {
  u32  ret_val   = network_word;
  u8   *src_ptr  = (u8 *)&network_word; //Point to front
  u8   *dest_ptr = ((u8 *)&ret_val) + sizeof(network_word) - 1; //Point to back
  u32  idx       = 0;

  //If not big endian, reverse byte order
  if( is_big_e == FALSE ) {
    for( ; idx < sizeof(network_word); idx++ ) {
      *dest_ptr-- = *src_ptr++; //Reverse the byte order
    } 
  } //else return word unmolested.

  return ret_val;
};

//Utility function to zero out validity structure
void clear_valid_payloads( frame_t *frame_data ) {
  //Fill structure with zeros (FALSE)
  memset( (void *) &(frame_data->valid), FALSE, sizeof( frame_data->valid ) );
}

//Public API
const char *SkypeECXU_ParserError( parse_error_t err_code ) {
  s32 idx = -err_code;
  return (idx >=0 && idx < PARSE_ERROR_CNT) ?
          PARSE_ERROR_STRINGS[idx]          :
          NULL;
}

parse_error_t SkypeECXU_ValidateFrame( void     *raw_frame   ,
                                       u32       frame_size  ,
                                       frame_t **parsed_frame ) {

  u8             *cur_ptr    = NULL; //Utility pointer
  u32             temp_word; //Holds current 32 bit word for various needs
  u16             temp_half_word; //Hold 16 bit half word for parsing
  skype_xu_hdr_t  cur_hdr;
  u32             idx        = 0;
  u32             num_hdrs   = 0;
  parse_error_t   error      = PARSE_NO_ERROR;
  u32             hdr_sec_size = 0, pload_sec_size = 0, free_space = 0;

  //Clear valid flags for payloads
  clear_valid_payloads( &cur_parsed_frame );
  *parsed_frame                    = NULL; //Init to NULL in case parser finds an error
  cur_parsed_frame.cur_payload_cnt = 0; //Clear current payload counter
  //Check for non-null frame
  if( raw_frame == NULL ) {
    error = PARSE_BAD_INPUT;
  }

  is_big_e = is_big_endian_l();

  //If there is no error, find the magic.
  if( error == PARSE_NO_ERROR ) {
    //Jump to end of stream and then back pointer up to look for magic
    cur_ptr   = (u8*)raw_frame + (frame_size-1); //point to back of stream
    cur_ptr   -= (sizeof(u32)-1); //Back up 32 bits to start.
    temp_word = 0;
    //Copy current 32 bits from network byte order (big endian) to host byte order
    temp_word = ntohl_l( *((u32 *)cur_ptr) ); //Copy current 32 bits to temp
    debug_printf( "Looking for magic...\n" );
    //Look for magic.  Stop if magic is found or buffer runs out of space to check.
    while( temp_word != MAGIC_GUID && cur_ptr > (u8 *)raw_frame ) {
      cur_ptr--;  //Move back one byte
      temp_word = ntohl_l( *((u32 *)cur_ptr) ); //Copy current 32 bits to temp
    }

    //Check for no magic error
    if( cur_ptr <= (u8 *)raw_frame ) {
      error = PARSE_NO_MAGIC;
      debug_printf( "No Magic\n" );
      cur_ptr = raw_frame; //Point to start of UVC frame to ensure no seg faults
    } else {
      debug_printf( "Magic found\n" );
    }
  }

  //Read number of headers if there is room in UVC frame and no previous error.
  if( error == PARSE_NO_ERROR && cur_ptr != NULL ) {
    free_space = cur_ptr - (u8 *)raw_frame; //Calculate bytes left in UVC frame
    //If there is enough free space to hold the header count, extract the count
    if( free_space > sizeof(num_hdrs) ) {
      cur_ptr -= sizeof(num_hdrs);
      num_hdrs = (u32)ntohl_l( *((u32 *)cur_ptr) );
      hdr_sec_size = num_hdrs * XU_HDR_SZ;
    } else {
      error = PARSE_NO_HEADER_COUNT;
      debug_printf( "Not enough room for header count\n" );
    }
  }

  //Check if header region overrides the available free space
  if( error == PARSE_NO_ERROR && hdr_sec_size > free_space && cur_ptr != NULL ) {
    error = PARSE_HDR_SEC_OVERFLOW;
    debug_printf( "Header section overruns UVC frame.\n" );
  } else if( error == PARSE_NO_ERROR ) {
    pload_sec_size = free_space - hdr_sec_size;
    cur_ptr       -= hdr_sec_size; //Back up to beginning of the header section
  }

  //If there are no errors, we are ready to process the headers and payloads
  if( error == PARSE_NO_ERROR && cur_ptr != NULL ) {
    debug_printf( "Number of headers = %X.\n", num_hdrs );
    
    //Get each header and fill in payload array
    idx = 0;
    while( idx < num_hdrs && error == PARSE_NO_ERROR ) {
      //Copy header data fields.  Each field is in big endian format.
      temp_word   = ntohl_l( *((u32 *)cur_ptr) ); //Grab first 32 bits
      cur_hdr.PTS = ((u64)temp_word);
      cur_hdr.PTS = (cur_hdr.PTS << 32);
      cur_ptr    += 4;
      temp_word   = ntohl_l( *((u32 *)cur_ptr) ); //Grab next 32 bits
      cur_hdr.PTS = cur_hdr.PTS | (u64)temp_word; //Or last 32 bits in place
      cur_ptr    += 4;
      //Grab 8 bit fields.  No need to swap
      cur_hdr.StreamID   = *cur_ptr;
      cur_ptr++;
      cur_hdr.StreamType = *cur_ptr;
      cur_ptr++;
      //Grab next fields with byte swapping
      temp_half_word        = ntohs_l( *((u16 *)cur_ptr) );
      cur_ptr              += sizeof(u16);
      cur_hdr.SequenceNum   = (u16)temp_half_word;
      cur_hdr.PayloadOffset = (u32)ntohl_l( *((u32 *)cur_ptr) );
      cur_ptr              += sizeof(u32);
      cur_hdr.PayloadSize   = (u32)ntohl_l( *((u32 *)cur_ptr) );
      cur_ptr              += sizeof(u32); //Next header now
      debug_printf( "Header %d data:\n  PTS: %X %X\n  SID: %d\n  Stp: %d\n  SqNum: %d\n  Poset: %X\n  Psz: %d\n\n",
                    idx, (u32)((cur_hdr.PTS) >> 32) & 0xFFFFFFF, (u32)(cur_hdr.PTS & 0xFFFFFFFF) , cur_hdr.StreamID,
                    cur_hdr.StreamType, cur_hdr.SequenceNum, cur_hdr.PayloadOffset, cur_hdr.PayloadSize );
      //Check that the current payload does not overrun the data boundary and
      //has no SID collisions
      if( cur_parsed_frame.valid[cur_hdr.StreamID] == TRUE ) {
        error = PARSE_SID_COLLISION;
      } else if( (cur_hdr.PayloadOffset + cur_hdr.PayloadSize) > pload_sec_size ) {
        error = PARSE_PAYLOAD_ERROR;
      } else { //No errors, copy data
        //Copy header data to user array
        cur_parsed_frame.parsed_payloads[cur_hdr.StreamID].sid     = cur_hdr.StreamID;
        cur_parsed_frame.parsed_payloads[cur_hdr.StreamID].s_type  = cur_hdr.StreamType;
        cur_parsed_frame.parsed_payloads[cur_hdr.StreamID].payload = (u8*)raw_frame + cur_hdr.PayloadOffset;
        cur_parsed_frame.parsed_payloads[cur_hdr.StreamID].PTS     = cur_hdr.PTS;
        cur_parsed_frame.parsed_payloads[cur_hdr.StreamID].seq_num = cur_hdr.SequenceNum;
        cur_parsed_frame.parsed_payloads[cur_hdr.StreamID].len     = cur_hdr.PayloadSize;
        cur_parsed_frame.valid          [cur_hdr.StreamID]         = TRUE;
        //If type is raw, grab the width and height.
        if( cur_hdr.StreamType == F_YUY2 || cur_hdr.StreamType == F_NV12 ) {
          cur_parsed_frame.parsed_payloads[cur_hdr.StreamID].resolution.width  = 
            ntohs_l( *((u16 *)(cur_parsed_frame.parsed_payloads[cur_hdr.StreamID].payload)) );
          cur_parsed_frame.parsed_payloads[cur_hdr.StreamID].resolution.height = 
            ntohs_l( *((u16 *)(cur_parsed_frame.parsed_payloads[cur_hdr.StreamID].payload)+1) );
        }
        cur_parsed_frame.cur_payload_cnt++;
        idx++;
      } //End if
    } //End while
  } //End if: PARSE_NO_ERROR

  //If no parsing errors were encountered return the parsed frame to the user
  if( error == PARSE_NO_ERROR ) {
    *parsed_frame = &cur_parsed_frame;
  }

  return( error );
}

parse_error_t SkypeECXU_GetNumberOfPayloads( frame_t *parsed_frame,
                                             u32     *num_streams  ) {

  parse_error_t error = PARSE_NO_ERROR;

  //If both pointers are non-null, copy payload count
  if( parsed_frame != NULL && num_streams != NULL ) {
    *num_streams = parsed_frame->cur_payload_cnt;
  //Otherwise indicate bad input and if possible set count to zero.
  } else if( num_streams != NULL ) {
    error = PARSE_BAD_INPUT;
    *num_streams = 0;
  } else {
    error = PARSE_BAD_INPUT;
  }

  return error;
}

parse_error_t SkypeECXU_ExtractPayload( u32         SID         ,
                                        frame_t    *parsed_frame,
                                        payload_t **req_payload  ) {

  parse_error_t error = PARSE_NO_ERROR;

  //If the passed input values are good, return the payload structure if valid
  if( parsed_frame != NULL ) {
    *req_payload =
      parsed_frame->valid[SID] == TRUE                                  ?
                                  &(parsed_frame->parsed_payloads[SID]) :
                                  NULL;
  } else {
    req_payload = NULL;
    error       = PARSE_BAD_INPUT;
  }

  return error;
}
