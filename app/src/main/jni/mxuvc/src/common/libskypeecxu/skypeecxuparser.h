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
*  Skype UVC XU Frame parser library interface specification.
*/
#ifndef __SKYPEECXUPARSER_H__
#define __SKYPEECXUPARSER_H__

#include "SkypeECXU.h" //Common definitions

#ifdef __cplusplus
extern "C" {
#endif

//#define DBG_PRINT 1

//Possible parsing errors
typedef enum {
  PARSE_NO_ERROR         =  0, //No errors
  PARSE_NO_MAGIC         = -1, //MAGIC not present in frame
  PARSE_NO_HEADER_COUNT  = -2, //Header count not present in frame
  PARSE_HDR_SEC_OVERFLOW = -3, //Header section overflows frame
  PARSE_PAYLOAD_ERROR    = -4, //Frame was not large enough to accommodate payloads
  PARSE_BAD_INPUT        = -5, //Function input variable bad
  PARSE_SID_COLLISION    = -6, //Multiple stream IDs found in single frame
  PARSE_ERROR_CNT        =  7, //Number of possible errors
} parse_error_t;

//Internal structure to hold parsed frame data.
typedef struct frame_s frame_t;

/******************************************************************************
* FUNCTION: SkypeECXU_ParserError
* INPUTS  : parse_error_t err_code - Error value returned from parser
*         :                          functions.
* OUTPUTS : None.
* RETURN  : const char * - String version of the error code.
* PURPOSE : Returns the human readable string version of the error codes
*         : returned by the parser functions.
******************************************************************************/
const char *SkypeECXU_ParserError( parse_error_t err_code );

/******************************************************************************
* FUNCTION: SkypeECXU_ValidateFrame
* INPUTS  : void *raw_frame  - Raw UVC frame exactly as received from driver.
*         : u32   frame_size - Size of raw frame.
* OUTPUTS : frame_t **parsed_frame - Internal structure containing the
*         :                          processed Skype UVC frame.  If there are
*         :                          any errors encountered during frame
*         :                          parsing, NULL is returned.  If parsing is
*         :                          successful parsed_frame contains the
*         :                          memory address of parsed frame data.
* RETURN  : parse_error_t - An enumerated value indicating any errors found
*         :                 during parsing.
* PURPOSE : Parse the UVC frame in the Skype camera format and return a
*         : reference to the parsed stream and return any errors
*         : encountered during stream processing.  The parsed frame pointer
*         : MUST be kept for passing to the other processing functions.  If
*         : parsing errors are encountered, all output values are NULL.
******************************************************************************/
parse_error_t SkypeECXU_ValidateFrame( void     *raw_frame   ,
                                       u32       frame_size  ,
                                       frame_t **parsed_frame );

/******************************************************************************
* FUNCTION: SkypeECXU_GetNumberOfPayloads
* INPUTS  : frame_t *parsed_frame - Pointer to the current parsed UVC frame.
* OUTPUTS : u32 num_streams - Number of streams present in payload.
* RETURN  : parse_error_t - An enumerated value indicating any errors found
*         :                 during parsing.  If any parsing errors are
*         :                 encountered, all output values are NULL.
* PURPOSE : Gets the number of frames present in the parsed frame.
******************************************************************************/
parse_error_t SkypeECXU_GetNumberOfPayloads( frame_t *parsed_frame,
                                             u32     *num_streams  );

/******************************************************************************
* FUNCTION: SkypeECXUGetPayload
* INPUTS  : u32 SID - Stream ID of the desired payload.
*         : frame_t *parsed_frame - Pointer to the current parsed UVC frame
* OUTPUTS : payload_t **req_payload - Pointer to the requested payload
*         :                           structure.  If there is not a current
*         :                           payload for the requested SID, then
*         :                           req_payload is returned as NULL.
* RETURN  : parse_error_t - An enumerated value indicating any errors found
*         :                 during parsing.  If any parsing errors are
*         :                 encountered, all output values are NULL.
* PURPOSE : Get the desired payload identified by its SID.
******************************************************************************/
parse_error_t SkypeECXU_ExtractPayload( u32         SID         ,
                                        frame_t    *parsed_frame,
                                        payload_t **req_payload  );

#ifdef __cplusplus
}; //End external C interface brace
#endif

#endif
