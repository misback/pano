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
* @(#) $Header: http://bgl-svn.local.geosemi.com/repos/swrepos/users/utkarsh/logo/condorsw/host/tools/f2l/yuvutil.c 52544 2015-02-27 08:34:49Z unimesh $
*******************************************************************************/
//#define DUMP 0
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "yuvutil.h"
#define LINEAR_FRAME_SIZE (8192*8192)
#define QCC_BID_PMU           0x21      /* Partition Management Unit */
#define QCC_BID_PMC           0x22      /* Physical Memory Ctl. (SDRAM) */

#define clip(x,l,h)     ( ((x)<(l))?(l):(((x)>(h))?(h):(x)) )

#define FULL_TILE      //Always write full tile if defined
                       //If not defined, write half/quarter tile for corner tiles if width/height of image not multiple of tile size
static int seg_hsize;
static int seg_vsize;
static int tile_hsize;
static int tile_vsize;
static int num_tileh;
static int num_tilev;

static char bInit = 0;

static void YUVUtil_Init();
static int DivRoundUp(int value, int divisor);
static inline void putTile(unsigned char *src, int stride, unsigned char* dst);
#ifndef FULL_TILE
static inline void putHalfTileH(unsigned char *src, int stride, unsigned char* dst);
#endif
static inline void putTileUV(unsigned char *U, unsigned char *V, int stride, unsigned char* dst);
#ifndef FULL_TILE
static inline void putHalfTileHUV(unsigned char *U, unsigned char *V, int stride, unsigned char* dst);
static inline void putHalfTileVUV(unsigned char *U, unsigned char *V, int stride, unsigned char* dst);
static inline void putQuarterTileUV(unsigned char *U, unsigned char *V, int stride, unsigned char* dst);
#endif

void Frame2LinearInit()
{
    YUVUtil_Init();
}

void Linear2FrameInit()
{
    YUVUtil_Init();
}

void l2f(char* tiled, int offsetx, int offsety, int width, int height, char* output)
{
    const int seg_w = (width  + seg_hsize - 1)&~(seg_hsize-1);
    const int seg_h = (height + seg_vsize - 1)&~(seg_vsize-1);
    const int seg_stride = seg_vsize*width;
    const int tile_stride = tile_vsize*width;
    char* out = output;
    int y, x, banky, bankx, tiley;

    for(y = offsety; y < seg_h; y+=seg_vsize, out+=seg_stride)
    {
        for(x = offsetx; x < seg_w; x+=seg_hsize, out+=seg_hsize)
        {
            for(banky = 0; banky < num_tilev; banky++, y+=tile_vsize, out+=tile_stride)
            {
                for(bankx = 0; bankx < num_tileh; bankx++, x+=tile_hsize, out+=tile_hsize)
                {
                    const int out_hsize = clip(width-x, 0, tile_hsize);
                    for(tiley = 0; tiley < tile_vsize; tiley++, y++, out+=width)
                    {
                        if(y < height) memcpy(out, tiled, out_hsize);
                        tiled += tile_hsize;
                    }
                    y -= tile_vsize; out -= tile_stride;
                }
                x -= seg_hsize; out -= seg_hsize;
            }
            y -= seg_vsize; out -= seg_stride;
        }
        out -= seg_w;
    }
}

void l2fUV(char* tiled, int offsetx, int offsety, int width, int height, char* u, char* v)
{
    const int seg_w = (2*width+ seg_hsize - 1)&~(seg_hsize-1);
    const int seg_h = (height + seg_vsize - 1)&~(seg_vsize-1);
    const int seg_stride = seg_vsize*width;
    const int tile_stride = tile_vsize*width;
    int off = 0;
    int y, x, banky, bankx, tiley, tilex;

    for(y = offsety; y < seg_h; y+=seg_vsize, off+=seg_stride)
    {
        for(x = offsetx; x < seg_w; x+=seg_hsize, off+=seg_hsize/2)
        {
            for(banky = 0; banky < num_tilev; banky++, y+=tile_vsize, off+=tile_stride)
            {
                for(bankx = 0; bankx < num_tileh; bankx++, x+=tile_hsize, off+=tile_hsize/2)
                {
                    const int out_hsize = clip(2*width-x, 0, tile_hsize);
                    for(tiley = 0; tiley < tile_vsize; tiley++, y++, off+=width)
                    {
                        if(y < height)
                        {
                            for(tilex = 0; tilex < out_hsize; tilex+=2 )
                            {
                                u[off+tilex/2] = tiled[tilex];
                                v[off+tilex/2] = tiled[tilex+1];
                            }
                        }
                        tiled += tile_hsize;
                    }
                    y -= tile_vsize; off -= tile_stride;
                }
                x -= seg_hsize; off -= seg_hsize/2;
            }
            y -= seg_vsize; off -= seg_stride;
        }
        off -= seg_w/2;
    }
}

int f2l(unsigned char* frame, int width, int height, unsigned char* output2, int bufferwidth)
{
    int i, j, m, n;
    unsigned char *pFrame;
    unsigned char *rowstart = output2;
    unsigned char *dst = rowstart;
    const int tile_size = tile_hsize*tile_vsize;
    int vpos, hpos;

    int physical_width = DivRoundUp(bufferwidth, tile_hsize * num_tileh) * tile_hsize * num_tileh;
    int tile_stride = physical_width * tile_vsize * num_tilev;

    for(i=0; i<height/seg_vsize; i++) // segment height
    {
        pFrame = &frame[seg_vsize * i * width];
        for(j=0; j<width/seg_hsize; j++) // segment width
        {
            for (m=0; m<num_tilev; m++)
            {
                for (n=0; n<num_tileh; n++)
                {
                    putTile(&pFrame[(n*tile_hsize)+(m*tile_vsize*width)], width, dst);
                    dst += tile_size;
                }
            }
            pFrame += seg_hsize;
        }
        n = 0;
        hpos = j * seg_hsize;
        // handle the last horizontal segment, if it's not divisble by width
        while ((hpos+tile_hsize) <= width)
        {
            for (m=0; m<num_tilev; m++)
            {
                putTile(&pFrame[(n*tile_hsize)+(m*tile_vsize*width)], width, dst+(m*num_tileh+n)*tile_size);
            }
            n++;
            hpos += tile_hsize;
        }
        if (hpos < width)
        {
            for (m=0; m<num_tilev; m++)
            {
#ifndef FULL_TILE
                putHalfTileH(&pFrame[(n*tile_hsize)+(m*tile_vsize*width)], width, dst+(m*num_tileh+n)*tile_size);
#else
                putTile(&pFrame[(n*tile_hsize)+(m*tile_vsize*width)], width, dst+(m*num_tileh+n)*tile_size);
#endif
            }
        }
        rowstart += tile_stride;
        dst = rowstart;
    }
    // handle the last vertical segment, if it's not divisble by height
    if ((i*seg_vsize) < height)
    {
        pFrame = &frame[seg_vsize * i * width];
        for (j=0; j<width/seg_hsize; j++)
        {
            m = 0;
            vpos = i * seg_vsize;
            while ((vpos+tile_vsize) <= height)
            {
                for (n=0; n<num_tileh; n++)
                {
                    putTile(&pFrame[(n*tile_hsize)+(m*tile_vsize*width)], width, dst+(m*num_tileh+n)*tile_size);
                }
                m++;
                vpos += tile_vsize;
            }
            pFrame += seg_hsize;
            dst += num_tileh * num_tilev * tile_size;
        }
        hpos = j * seg_hsize;
        if (hpos < width)
        {
            n = 0;
            while((hpos+tile_hsize) <= width)
            {
                m = 0;
                vpos = i * seg_vsize;
                while ((vpos + tile_vsize) <= height)
                {
                    putTile(&pFrame[(n*tile_hsize)+(m*tile_vsize*width)], width, dst+(m*num_tileh+n)*tile_size);
                    m++;
                    vpos += tile_vsize;
                }
                n++;
                hpos += tile_hsize;
            }
            if (hpos < width)
            {
                m = 0;
                vpos = i * seg_vsize;
                while ((vpos + tile_vsize) <= height)
                {
#ifndef FULL_TILE
                    putHalfTileH(&pFrame[(n*tile_hsize)+(m*tile_vsize*width)], width, dst+(m*num_tileh+n)*tile_size);
#else
                    putTile(&pFrame[(n*tile_hsize)+(m*tile_vsize*width)], width, dst+(m*num_tileh+n)*tile_size);
#endif
                    m++;
                    vpos += tile_vsize;
                }
            }
            dst += num_tileh * num_tilev * tile_size;
        }
    }
    // end block move
    return dst-output2;
}

int f2lUV(unsigned char* U, unsigned char* V, int width, int height, unsigned char* output2, int bufferwidth)
{
    unsigned char* rowstart = output2;
    unsigned char* dst = rowstart;
    const int tile_size = tile_hsize*tile_vsize;
    const int half_tile_hsize = (tile_hsize >> 1);

    int physical_width = DivRoundUp(bufferwidth, tile_hsize * num_tileh) * tile_hsize * num_tileh;
    int tile_stride = physical_width * tile_vsize * num_tilev;

#define A(o) pU+o, pV+o
    int i, j, m, n;
    int hpos, vpos;
    unsigned char* pU;
    unsigned char* pV;
    for(i=0; i < height/seg_vsize; i++) // segment height
    {
        pU = &U[seg_vsize*width*i];
        pV = &V[seg_vsize*width*i];
        for(j=0; j < (2*width/seg_hsize); j++) // segment width
        {
            for (m=0; m<num_tilev; m++)
            {
                for (n=0; n<num_tileh; n++)
                {
                    putTileUV(A((m*tile_vsize*width)+(n*half_tile_hsize)), width, dst);
                    dst += tile_size;
                }
            }
            pU += (seg_hsize >> 1); pV += (seg_hsize >> 1);
        }
        n = 0;
        hpos = j * (seg_hsize >> 1);
        // handle the last horizontal segment, if it's not divisible by width
        while ((hpos+half_tile_hsize) <= width)
        {
            for (m=0; m<num_tilev; m++)
            {
                putTileUV(A((m*tile_vsize*width)+(n*half_tile_hsize)), width, dst+(m*num_tileh+n)*tile_size);
            }
            n++;
            hpos += half_tile_hsize;
        }
        if (hpos < width)
        {
            for (m=0; m<num_tilev; m++)
            {
#ifndef FULL_TILE
                putHalfTileHUV(A((m*tile_vsize*width)+(n*half_tile_hsize)), width, dst+(m*num_tileh+n)*tile_size);
#else
                putTileUV(A((m*tile_vsize*width)+(n*half_tile_hsize)), width, dst+(m*num_tileh+n)*tile_size);
#endif
            }
        }
        rowstart += tile_stride;
        dst = rowstart;
    }
    // handle the last vertical segment, if it's not divisible by height
    if ((i * seg_vsize) < height)
    {
        pU = &U[seg_vsize*width*i];
        pV = &V[seg_vsize*width*i];
        for (j=0; j < (2*width/seg_hsize); j++)
        {
            m = 0;
            vpos = i * seg_vsize;
            while ((vpos + tile_vsize) <= height)
            {
                for (n=0; n<num_tileh; n++)
                {
                    putTileUV(A((m*tile_vsize*width)+(n*half_tile_hsize)), width, dst+(m*num_tileh+n)*tile_size);
                }
                m++;
                vpos += tile_vsize;
            }
            if (vpos < height)
            {
                for (n=0; n<num_tileh; n++)
                {
#ifndef FULL_TILE
                    putHalfTileVUV(A((m*tile_vsize*width)+(n*half_tile_hsize)), width, dst+(m*num_tileh+n)*tile_size);
#else
                    putTileUV(A((m*tile_vsize*width)+(n*half_tile_hsize)), width, dst+(m*num_tileh+n)*tile_size);
#endif
                }
            }
            dst += num_tileh * num_tilev * tile_size;
            pU += (seg_hsize >> 1); pV += (seg_hsize >> 1);
        }
        hpos = j * (seg_hsize >> 1);
        if (hpos < width)
        {
            n = 0;
            while ((hpos+half_tile_hsize) <= width)
            {
                m = 0;
                vpos = i * seg_vsize;
                while ((vpos+tile_vsize) <= height)
                {
                    putTileUV(A((m*tile_vsize*width)+(n*half_tile_hsize)), width, dst+(m*num_tileh+n)*tile_size);
                    m++;
                    vpos += tile_vsize;
                }
                if (vpos < height)
                {
#ifndef FULL_TILE
                    putHalfTileVUV(A((m*tile_vsize*width)+(n*half_tile_hsize)), width, dst+(m*num_tileh+n)*tile_size);
#else
                    putTileUV(A((m*tile_vsize*width)+(n*half_tile_hsize)), width, dst+(m*num_tileh+n)*tile_size);
#endif
                }
                n++;
                hpos += half_tile_hsize;
            }
            if (hpos < width)
            {
                m = 0;
                vpos = i * seg_vsize;
                while ((vpos+tile_vsize) <= height)
                {
#ifndef FULL_TILE
                    putHalfTileHUV(A((m*tile_vsize*width)+(n*half_tile_hsize)), width, dst+(m*num_tileh+n)*tile_size);
#else
                    putTileUV(A((m*tile_vsize*width)+(n*half_tile_hsize)), width, dst+(m*num_tileh+n)*tile_size);
#endif
                    m++;
                    vpos += tile_vsize;
                }
                if (vpos < height)
                {
#ifndef FULL_TILE
                    putQuarterTileUV(A((m*tile_vsize*width)+(n*half_tile_hsize)), width, dst+(m*num_tileh+n)*tile_size);
#else
                    putTileUV(A((m*tile_vsize*width)+(n*half_tile_hsize)), width, dst+(m*num_tileh+n)*tile_size);
#endif
                }
            }
            dst += num_tileh * num_tilev * tile_size;
        }
    }
#undef A
    // end block move
    return dst-output2;
}

void YUVUtil_GetSegmentSize(int *width, int *height)
{
    if (bInit == 1)
    {
        *width = seg_hsize;
        *height = seg_vsize;
    }
    else
    {
        *width = 0;
        *height = 0;
        printf("YUVUtil_GetSegmentSize Error: Need to initialize first\n");
    }
}

static void YUVUtil_Init()
{
    // constants for seve1
    static const int  VPAT_SXSW =  9;         // Segment     Horizontal Size    <=  512 bytes
    static const int  VPAT_SYSW =  6;         // Segment     Vertical   Size    <=   64 lines

    //unsigned long data = 0;
    
    int VPATSegXSize, VPATSegYSize;
    int WordSize;
    int TileXSize, TileYSize;
    int BankXSize, BankYSize;
    int SegXSize, SegYSize;
    
    if (bInit == 0)
    {
        VPATSegXSize = 2;
        VPATSegYSize = 2;
	    WordSize = 3;
        TileXSize = 3;
        TileYSize = 4;
        BankXSize = 2;
        BankYSize = 1;
        SegXSize = VPAT_SXSW - 3 + VPATSegXSize;
        SegYSize = VPAT_SYSW - 3 + VPATSegYSize;
    
        // calculations for SwizzleFrameAddr
        seg_hsize = 1 << SegXSize;
        seg_vsize = 1 << SegYSize;
    
        tile_hsize = 1 << (TileXSize + WordSize);
        tile_vsize = 1 <<  TileYSize;
    
        num_tileh  = 1 <<  BankXSize;
        num_tilev  = 1 <<  BankYSize;
        
        bInit = 1;

    }
}

void YUVUtil_Init2(unsigned int reg1, unsigned int reg2, unsigned int reg3, unsigned int reg4)
{
    // constants for seve1
    static const int  VPAT_SXSW =  9;         // Segment     Horizontal Size    <=  512 bytes
    static const int  VPAT_SYSW =  6;         // Segment     Vertical   Size    <=   64 lines

    unsigned int data = 0;
    
    int VPATSegXSize, VPATSegYSize;
    int WordSize;
    int TileXSize, TileYSize;
    int BankXSize, BankYSize, PageSize;
    int SegXSize, SegYSize;
    
    if (bInit == 0)
    {
        data = reg1;
        VPATSegXSize = data & 0x3;
        VPATSegYSize = (data & 0xc) >> 2;

	    data = reg2;
        WordSize = data & 0x7;

	    data = reg3;
        TileXSize = (data & 0x70) >> 4;
        TileYSize = (data & 0x7);

	    data = reg4;

        BankXSize = (data & 0xc) >> 2;
        BankYSize = (data & 0x3);
        PageSize = (data & 0x70) >> 4;

        SegXSize = VPAT_SXSW - 3 + VPATSegXSize;
        SegYSize = VPAT_SYSW - 3 + VPATSegYSize;


        SegXSize = VPAT_SXSW - 3 + VPATSegXSize;
        SegYSize = VPAT_SYSW - 3 + VPATSegYSize;
    
        // calculations for SwizzleFrameAddr
        seg_hsize = 1 << SegXSize;
        seg_vsize = 1 << SegYSize;
    
        tile_hsize = 1 << (TileXSize + WordSize);
        tile_vsize = 1 <<  TileYSize;
    
        num_tileh  = 1 <<  BankXSize;
        num_tilev  = 1 <<  BankYSize;
        
#if 1
        printf( "VPATSegXSize %d VPATSegYSize %d\n", VPATSegXSize, VPATSegYSize );
        printf( "WordSize %d\n", WordSize );
        printf( "TileXSize %d TileYSize %d\n", TileXSize, TileYSize );
        printf( "BankXSize %d BankYSize %d PageSize %d\n", BankXSize, BankYSize, PageSize );
        printf( "seg_hsize %d seg_vsize %d\n", seg_hsize, seg_vsize );
        printf( "tile_hsize %d tile_vsize %d\n", tile_hsize, tile_vsize );
        printf( "num_tileh %d num_tilev %d\n", num_tileh, num_tilev );
#endif
        bInit = 1;
        
    }
}

static int DivRoundUp(int value, int divisor)
{
    return ((value + divisor - 1)/divisor);
}

static inline void putTile(unsigned char *src, int stride, unsigned char* dst)
{
    const int x = tile_hsize, y = tile_vsize;
    int i;
    for(i=0;i<y;i++) {
        memcpy(dst, src, x);
        dst += x; src += stride;
    }
}

#ifndef FULL_TILE
static inline void putHalfTileH(unsigned char *src, int stride, unsigned char* dst)
{
    const int x = tile_hsize, y = tile_vsize;
    int i;
    for(i=0;i<y;i++) {
        memcpy(dst, src, x/2/*only change*/);
        dst += x; src += stride;
    }
}
#endif

static inline void putTileUV(unsigned char *U, unsigned char *V, int stride, unsigned char* dst)
{
    const int x = tile_hsize, y = tile_vsize;
    unsigned char buf[x];
    int i, j;
    for(i=0;i<y;i++) {
        for(j=0; j<x/2; j++) { // interleave chroma samples
            buf[2*j] = U[j];
            buf[2*j+1] = V[j];
        }
        memcpy(dst, buf, x);
        dst += x;  U += stride; V += stride;
    }
}

static inline void putHalfTileHUV(unsigned char *U, unsigned char *V, int stride, unsigned char* dst)
{
    const int x = tile_hsize, y = tile_vsize;
    unsigned char buf[x];
    int i, j;
    for(i=0;i<y;i++) {
        for(j=0; j<x/4; j++) { // interleave chroma samples
            buf[2*j] = U[j];
            buf[2*j+1] = V[j];
        }
        memcpy(dst, buf, x/2);
        dst += x;  U += stride; V += stride;
    }
}

static inline void putHalfTileVUV(unsigned char *U, unsigned char *V, int stride, unsigned char* dst)
{
    const int x = tile_hsize, y = tile_vsize;
    unsigned char buf[x];
    int i, j;
    for(i=0;i<y/2;i++) {
        for(j=0; j<x/2; j++) { // interleave chroma samples
            buf[2*j] = U[j];
            buf[2*j+1] = V[j];
        }
        memcpy(dst, buf, x);
        dst += x;  U += stride; V += stride;
    }
}

static inline void putQuarterTileUV(unsigned char *U, unsigned char *V, int stride, unsigned char* dst)
{
    const int x = tile_hsize, y = tile_vsize;
    unsigned char buf[x];
    int i, j;
    for(i=0;i<y/2;i++) {
        for(j=0; j<x/4; j++) { // interleave chroma samples
            buf[2*j] = U[j];
            buf[2*j+1] = V[j];
        }
        memcpy(dst, buf, x/2);
        dst += x;  U += stride; V += stride;
    }
}


int WriteVideoFrame(FILE *fpi, int width, int height, int bufferwidth, unsigned char** ybuf, unsigned char** uvbuf, int* ysize, int* uvsize, int YUVformat , int startframe)
{
    int x, y;
    unsigned char * fbuffer=0;
    unsigned char * bufferY=0;
    unsigned char * bufferU=0;
    unsigned char * bufferV=0;
    unsigned char * linebuffer=0;
    int maxaddr=0;
    int bytesread=0;
    int chroma_width=0;
    int chroma_height=0;
    int chroma_bufferwidth=0;
    unsigned char *yptr = 0, *uptr = 0, *vptr = 0, *lineptr = 0;
    int size =0,sizec=0, framesize=0;
    long long offset=0;
    int planar=1;
    int linearFrameSize=0;      //Round to horizontal and vertical segment size

    printf("width %d height %d bufferwidth %d yuvformat %d ", width, height, bufferwidth, YUVformat); // print the format in the case statement

    

    bufferY    = (unsigned char *)malloc(width*height);
    if(!bufferY)
    {
        free(fbuffer);
        return -1;
    }
    
    
    size  = width * height;
    chroma_bufferwidth = bufferwidth;

    switch (YUVformat)
    {
    	case 0:  // YUV420 Planar (MPEG)
		printf("(YUV420 Planar)\n");
		chroma_width = width/2;
		chroma_height = height/2;
                
		break;
    	case 1:  // YUV422 Interleaved (ITU656)
		planar = 0;
		printf("(YUV422 Interleaved)\n");
		chroma_width = width/2;
		chroma_height = height;

		break;
    	case 2:  // YUV422 Planar
		printf("(YUV422 Planar)\n");
		chroma_width = width/2;
		chroma_height = height;
                
		break;
    	case 3:  // YUV411 Planar 
		printf("(YUV411 Planar)\n");
		chroma_width = width/4;
		chroma_height = height;
		
		chroma_bufferwidth /= 2; 

		break;
    	case 4:  // YUV444 Planar (Graphics)
		printf("(YUV444 Planar)\n");
		chroma_width = width;
		chroma_height = height;

		chroma_bufferwidth *= 2; 
                
		break;
    	default: // YUV420 Planar (MPEG)
		printf("(YUV420 Planar)\n");
		chroma_width = width/2;
		chroma_height = height/2;
		break;
    }

    bufferU    = (unsigned char *)malloc(chroma_width*chroma_height);
    if(!bufferU)
    {
        free(fbuffer);
        free(bufferY);
        return -1;
    }
    bufferV    = (unsigned char *)malloc(chroma_width*chroma_height);
    if(!bufferV)
    {
        free(fbuffer);
        free(bufferY);
        free(bufferU);
        return -1;
    }

    if (planar)
    {

		sizec = chroma_width * chroma_height;
		framesize =  size + 2*sizec;
		offset = (long long)startframe*(long long)framesize;
		if(fseeko(fpi, offset, 0)) 
		{
		    printf("Fseek failed.\n");
		    return(1);
		}
		
    		bytesread = fread(bufferY, 1, width*height, fpi);
    		if( bytesread == width*height ) 
                bytesread = fread(bufferU, 1, chroma_width*chroma_height, fpi);
    		if( bytesread == chroma_width*chroma_height ) 
                bytesread = fread(bufferV, 1, chroma_width*chroma_height, fpi);
    }
    else // interleaved
    {
        linebuffer = (unsigned char *)malloc(2*width);
        if(!linebuffer)
        {
            free(fbuffer);
            free(bufferY);
            free(bufferU);
            free(bufferV);
            return -1;
        }
		sizec = chroma_width * chroma_height;
		framesize =  size + 2*sizec;
		offset = (long long)startframe*(long long)framesize;
		if(fseeko(fpi, offset, 0)) 
		{
		    printf("Fseek failed.\n");
		    return(1);
		}

		yptr    = bufferY;
		uptr    = bufferU;
		vptr    = bufferV;

		for (y=0; y < height; y++)
		{
    			bytesread = fread(linebuffer, 1, 2*width, fpi);
			lineptr = linebuffer;
			for (x=0; x < width/2; x++)
			{
				*uptr++ = *lineptr++;
				*yptr++ = *lineptr++;
				*vptr++ = *lineptr++;
				*yptr++ = *lineptr++;
			}
		}
    }

    // check for eof
    if (bytesread == 0)
    {
        // free the memory
        free(fbuffer);
        free(bufferY);
        free(bufferU);
        free(bufferV);
        
        return(1);
    } 
    // To deal with non multiple of 16 heights. Fake it to think its a multiple of 16. You dont care abt the extra lines that is put
    if(height%16) {
    	height = ((height+15) >> 4) << 4;
        printf("Height %d\n", height);    

    }

    //in case overlay does not do a init use defaults	
    if(bInit == 0)	
    Frame2LinearInit();
    
    // malloc frame buffer
    linearFrameSize = ((width + seg_hsize - 1) & ~(seg_hsize-1)) * ((height + seg_vsize - 1) & ~(seg_vsize-1));  
    printf("size %d seg_hsize %d seg_vsize %d ", linearFrameSize, seg_hsize, seg_vsize);

    fbuffer    = (unsigned char *)malloc(linearFrameSize);

    if(!fbuffer)
    {
        free(bufferY);
        free(bufferU);
        free(bufferV);
        return -1;
    }
        

    maxaddr = f2l( bufferY, width, height, fbuffer, bufferwidth);
    fprintf(stderr, "maxadddr y %d\n", maxaddr);
    *ybuf = (unsigned char *)malloc(maxaddr);
    *ysize = maxaddr;
    memcpy(*ybuf,fbuffer, maxaddr);

    #ifdef DUMP
    FILE* fpoy = fopen("y.out", "wb");
    fwrite(fbuffer, 1, maxaddr, fpoy);
    fclose(fpoy);
    #endif
    // To deal with non multiple of 8 heights. Fake it to think its a multiple of 8. You dont care abt the extra lines that is put
    if(chroma_height%8) {
    	chroma_height = ((chroma_height+7) >> 3) << 3;
        printf("chroma_height %d\n", chroma_height);

    }

    maxaddr = f2lUV( bufferU, bufferV, chroma_width, chroma_height, fbuffer, chroma_bufferwidth );
    fprintf(stderr, "maxadddr uv %d\n", maxaddr);
    //fwrite(fbuffer, 1, maxaddr, fpouv);
    *uvbuf = (unsigned char *)malloc(maxaddr);
    *uvsize = maxaddr;
    memcpy(*uvbuf,fbuffer, maxaddr);
    #ifdef DUMP
    FILE* fpouv = fopen("uv.out", "wb");
    fwrite(fbuffer, 1, maxaddr, fpouv);
    fclose(fpouv);
    #endif

    // free the buffer
    free(fbuffer);
    free(bufferY);
    free(bufferU);
    free(bufferV);
    free(linebuffer);
    return(1);
}

int  ConvertFrameToTile(unsigned char *pbuf, int width, int height, int bufferwidth, unsigned char** ybuf, unsigned char** uvbuf, int* ysize, int* uvsize, int YUVformat , int startframe)
{
    int x, y;
    unsigned char * fbuffer=0;
    unsigned char * bufferY=0;
    unsigned char * bufferU=0;
    unsigned char * bufferV=0;
    unsigned char * linebuffer=0;
    int maxaddr=0;
    int chroma_width=0;
    int chroma_height=0;
    int chroma_bufferwidth=0;
    unsigned char *yptr = 0, *uptr = 0, *vptr = 0, *lineptr = 0;
    int size =0,sizec=0, framesize=0;
    long long offset=0;
    int planar=1;
    int linearFrameSize=0;      //Round to horizontal and vertical segment size

    
    printf("width %d height %d bufferwidth %d yuvformat %d ", width, height, bufferwidth, YUVformat); // print the format in the case statement

    bufferY    = (unsigned char *)malloc(width*height);
    if(!bufferY)
    {
        free(fbuffer);
        return -1;
    }
    
    
    size  = width * height;
    chroma_bufferwidth = bufferwidth;

    switch (YUVformat)
    {
        case 0:  // YUV420 Planar (MPEG)
        printf("(YUV420 Planar)\n");
        chroma_width = width/2;
        chroma_height = height/2;
        break;
        case 1:  // YUV422 Interleaved (ITU656)
        planar = 0;
        printf("(YUV422 Interleaved)\n");
        chroma_width = width/2;
        chroma_height = height;
        break;
        case 2:  // YUV422 Planar
        printf("(YUV422 Planar)\n");
        chroma_width = width/2;
        chroma_height = height;
                
        break;
        case 3:  // YUV411 Planar 
        printf("(YUV411 Planar)\n");
        chroma_width = width/4;
        chroma_height = height;
        chroma_bufferwidth /= 2; // Achtung! Gefahr, Wille Robinson!

        break;
        case 4:  // YUV444 Planar (Graphics)
        printf("(YUV444 Planar)\n");
        chroma_width = width;
        chroma_height = height;
        chroma_bufferwidth *= 2; // Achtung! Gefahr, Wille Robinson!
                
        break;
        default: // YUV420 Planar (MPEG)
        printf("(YUV420 Planar)\n");
        chroma_width = width/2;
        chroma_height = height/2;
        break;
    }


    bufferU    = (unsigned char *)malloc(chroma_width*chroma_height);
    if(!bufferU)
    {
        free(fbuffer);
        free(bufferY);
        return -1;
    }
    bufferV    = (unsigned char *)malloc(chroma_width*chroma_height);
    if(!bufferV)
    {
        free(fbuffer);
        free(bufferY);
        free(bufferU);
        return -1;
    }

    if (planar)
    {
        sizec = chroma_width * chroma_height;
        framesize =  size + 2*sizec;
        offset = (long long)startframe*(long long)framesize;
        printf("copying Y comp\n");
        memcpy(bufferY, &pbuf[offset], width*height);

        printf("copying U comp\n");
        memcpy(bufferU, &pbuf[offset+(width*height)], sizec);

        printf("copying V comp\n");
        memcpy(bufferV, &pbuf[offset+(width*height)+sizec], sizec);
    }
    else // interleaved
    {
        linebuffer = (unsigned char *)malloc(2*width);
        if(!linebuffer)
        {
            free(fbuffer);
            free(bufferY);
            free(bufferU);
            free(bufferV);
            return -1;
        }
        sizec = chroma_width * chroma_height;
        framesize =  size + 2*sizec;
        offset = (long long)startframe*(long long)framesize;

        yptr    = bufferY;
        uptr    = bufferU;
        vptr    = bufferV;

        for (y=0; y < height; y++)
        {
            memcpy(linebuffer, &pbuf[offset], 2*width);
            lineptr = linebuffer;
            for (x=0; x < width/2; x++)
            {
                *uptr++ = *lineptr++;
                *yptr++ = *lineptr++;
                *vptr++ = *lineptr++;
                *yptr++ = *lineptr++;
            }
        }
    }
   
    // To deal with non multiple of 16 heights. Fake it to think its a multiple of 16. You dont care abt the extra lines that is put
    if(height%16) {
        height = ((height+15) >> 4) << 4;
        printf("Height %d\n", height);    
    }

    // malloc frame buffer
    linearFrameSize = ((width + seg_hsize - 1) & ~(seg_hsize-1)) * ((height + seg_vsize - 1) & ~(seg_vsize-1));  
    printf("size %d seg_hsize %d seg_vsize %d ", linearFrameSize, seg_hsize, seg_vsize);

    fbuffer    = (unsigned char *)malloc(linearFrameSize);
    
    if(!fbuffer)
    {
        free(bufferY);
        free(bufferU);
        free(bufferV);
        return -1;
    }
    maxaddr = f2l( bufferY, width, height, fbuffer, bufferwidth);
    printf("maxadddr %p, ybuf %d\n", *ybuf, maxaddr);

    *ysize = maxaddr;
    memcpy(*ybuf,fbuffer, maxaddr);

    // To deal with non multiple of 8 heights. Fake it to think its a multiple of 8. 
    // Dont care abt the extra lines that is put
    if(chroma_height%8) {
        chroma_height = ((chroma_height+7) >> 3) << 3;
        printf("chroma_height %d\n", chroma_height);
    }

    maxaddr = f2lUV( bufferU, bufferV, chroma_width, chroma_height, fbuffer, chroma_bufferwidth );
    printf("maxadddr %p, uv %d\n", *uvbuf, maxaddr);
    
    *uvsize = maxaddr;
    memcpy(*uvbuf,fbuffer, maxaddr);

    // free the buffer
    free(fbuffer);
    free(bufferY);
    free(bufferU);
    free(bufferV);
    free(linebuffer);
    return(1);
}