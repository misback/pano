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
* @(#) $Header: http://bgl-svn.local.geosemi.com/repos/swrepos/users/utkarsh/logo/condorsw/host/tools/f2l/yuvutil.h 52544 2015-02-27 08:34:49Z unimesh $
*******************************************************************************/

#ifndef __YUVUTIL_H
#define __YUVUTIL_H

void Frame2LinearInit();
void Linear2FrameInit();

void l2f(char* tiled, int offsetx, int offsety, int width, int height, char* output);
void l2fUV(char* tiled, int offsetx, int offsety, int width, int height, char* u, char* v);
int  f2l(unsigned char* frame, int width, int height, unsigned char* output2, int bufferwidth);
int  f2lUV(unsigned char* U, unsigned char* V, int width, int height, unsigned char* output2, int bufferwidth);
int  WriteVideoFrame(FILE *fpi, int width, int height, int bufferwidth, unsigned char** y, unsigned char** uv, int* ysize, int* uvsize, int YUVformat , int startframe);
int  ConvertFrameToTile(unsigned char *pbuf, int width, int height, int bufferwidth, unsigned char** y, unsigned char** uv, int* ysize, int* uvsize, int YUVformat , int startframe);

void YUVUtil_GetSegmentSize(int *width, int *height);
void YUVUtil_Init2(unsigned int reg1, unsigned int reg2, unsigned int reg3, unsigned int reg4);
#endif

