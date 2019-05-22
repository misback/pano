//
// Created by roading on 2016/6/24.
//
#include "Bitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//生成BMP图片(无颜色表的位图):在RGB(A)位图数据的基础上加上文件信息头和位图信息头
int GenBmpFile(U8 *pData, U8 bitCountPerPix, U32 width, U32 height, const char *filename);
//获取BMP文件的位图数据(无颜色表的位图):丢掉BMP文件的文件信息头和位图信息头，获取其RGB(A)位图数据
U8* GetBmpData(U8 *bitCountPerPix, U32 *width, U32 *height, const char* filename);
//释放GetBmpData分配的空间
void FreeBmpData(U8 *pdata);

void GetBmpInfo (U8 *bitCountPerPix, U32 *width, U32 *height, U8* bitmapData );

//生成BMP图片(无颜色表的位图):在RGB(A)位图数据的基础上加上文件信息头和位图信息头
int GenBmpFile(U8 *pData, U8 bitCountPerPix, U32 width, U32 height, const char *filename){
    FILE *fp = fopen(filename, "wb");
    if(!fp){
        printf("fopen failed : %s, %d\n", __FILE__, __LINE__);
        return 0;
    }
    U32 bmppitch = ((width*bitCountPerPix + 31) >> 5) << 2;
    U32 filesize = bmppitch*height;
    BITMAPFILE bmpfile;
    bmpfile.bfHeader.bfType = 0x4D42;
    bmpfile.bfHeader.bfSize = filesize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmpfile.bfHeader.bfReserved1 = 0;
    bmpfile.bfHeader.bfReserved2 = 0;
    bmpfile.bfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmpfile.biInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpfile.biInfo.bmiHeader.biWidth = width;
    bmpfile.biInfo.bmiHeader.biHeight = height;
    bmpfile.biInfo.bmiHeader.biPlanes = 1;
    bmpfile.biInfo.bmiHeader.biBitCount = bitCountPerPix;
    bmpfile.biInfo.bmiHeader.biCompression = 0;
    bmpfile.biInfo.bmiHeader.biSizeImage = 0;
    bmpfile.biInfo.bmiHeader.biXPelsPerMeter = 0;
    bmpfile.biInfo.bmiHeader.biYPelsPerMeter = 0;
    bmpfile.biInfo.bmiHeader.biClrUsed = 0;
    bmpfile.biInfo.bmiHeader.biClrImportant = 0;
    fwrite(&(bmpfile.bfHeader), sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&(bmpfile.biInfo.bmiHeader), sizeof(BITMAPINFOHEADER), 1, fp);
    U8 *pEachLinBuf = (U8*)malloc(bmppitch);
    memset(pEachLinBuf, 0, bmppitch);
    U8 BytePerPix = bitCountPerPix >> 3;
    U32 pitch = width * BytePerPix;
    if(pEachLinBuf){
        int h,w;
        for(h = height-1; h >= 0; h--){
            for(w = 0; w < width; w++){
                //copy by a pixel
                pEachLinBuf[w*BytePerPix+0] = pData[h*pitch + w*BytePerPix + 0];
                pEachLinBuf[w*BytePerPix+1] = pData[h*pitch + w*BytePerPix + 1];
                pEachLinBuf[w*BytePerPix+2] = pData[h*pitch + w*BytePerPix + 2];
            }
            fwrite(pEachLinBuf, bmppitch, 1, fp);
        }
        free(pEachLinBuf);
    }
    fflush(fp);
    fclose(fp);
    return 1;
}

int GenBmpFile(U8 *pData, U32 width, U32 height, const char *filename){
    FILE *fp = fopen(filename, "wb");
    if(!fp){
        printf("fopen failed : %s, %d\n", __FILE__, __LINE__);
        return 0;
    }
    fwrite(pData, 1, width*height, fp);
    fflush(fp);
    fclose(fp);
    return 1;
}
int GenBmpFile_Gray(U8 *pData, U8 bitCountPerPix, U32 width, U32 height, const char *filename){
    FILE *fp = fopen(filename, "wb");
    if(!fp){
        printf("fopen failed : %s, %d\n", __FILE__, __LINE__);
        return 0;
    }
    U32 bmppitch = ((width*bitCountPerPix + 31) >> 5) << 2;
    U32 filesize = bmppitch*height;
    BITMAPFILE bmpfile;
    bmpfile.bfHeader.bfType = 0x4D42;
    bmpfile.bfHeader.bfSize = filesize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmpfile.bfHeader.bfReserved1 = 0;
    bmpfile.bfHeader.bfReserved2 = 0;
    bmpfile.bfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmpfile.biInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpfile.biInfo.bmiHeader.biWidth = width;
    bmpfile.biInfo.bmiHeader.biHeight = height;
    bmpfile.biInfo.bmiHeader.biPlanes = 1;
    bmpfile.biInfo.bmiHeader.biBitCount = bitCountPerPix;
    bmpfile.biInfo.bmiHeader.biCompression = 0;
    bmpfile.biInfo.bmiHeader.biSizeImage = 0;
    bmpfile.biInfo.bmiHeader.biXPelsPerMeter = 0;
    bmpfile.biInfo.bmiHeader.biYPelsPerMeter = 0;
    bmpfile.biInfo.bmiHeader.biClrUsed = 0;
    bmpfile.biInfo.bmiHeader.biClrImportant = 0;
    fwrite(&(bmpfile.bfHeader), sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&(bmpfile.biInfo.bmiHeader), sizeof(BITMAPINFOHEADER), 1, fp);
    U8 *pEachLinBuf = (U8*)malloc(bmppitch);
    memset(pEachLinBuf, 0, bmppitch);
    U8 BytePerPix = 1;
    U32 pitch = width * BytePerPix;
    if(pEachLinBuf){
        int h,w;
        for(h = 0; h <height; h++){
            for(w = 0; w < width; w++){
                //copy by a pixel
                pEachLinBuf[w*4+0] = pData[h*pitch + w*BytePerPix + 0];
                pEachLinBuf[w*4+1] = pData[h*pitch + w*BytePerPix + 0];
                pEachLinBuf[w*4+2] = pData[h*pitch + w*BytePerPix + 0];
            }
            fwrite(pEachLinBuf, bmppitch, 1, fp);
        }
        free(pEachLinBuf);
    }
    fflush(fp);
    fclose(fp);
    return 1;
}

int GenBmpFile_BGR(U8 *pData, U8 bitCountPerPix, U32 width, U32 height, const char *filename){
    FILE *fp = fopen(filename, "wb");
    if(!fp){
        printf("fopen failed : %s, %d\n", __FILE__, __LINE__);
        return 0;
    }
    U32 bmppitch = ((width*bitCountPerPix + 31) >> 5) << 2;
    U32 filesize = bmppitch*height;
    BITMAPFILE bmpfile;
    bmpfile.bfHeader.bfType = 0x4D42;
    bmpfile.bfHeader.bfSize = filesize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmpfile.bfHeader.bfReserved1 = 0;
    bmpfile.bfHeader.bfReserved2 = 0;
    bmpfile.bfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmpfile.biInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpfile.biInfo.bmiHeader.biWidth = width;
    bmpfile.biInfo.bmiHeader.biHeight = height;
    bmpfile.biInfo.bmiHeader.biPlanes = 1;
    bmpfile.biInfo.bmiHeader.biBitCount = bitCountPerPix;
    bmpfile.biInfo.bmiHeader.biCompression = 0;
    bmpfile.biInfo.bmiHeader.biSizeImage = 0;
    bmpfile.biInfo.bmiHeader.biXPelsPerMeter = 0;
    bmpfile.biInfo.bmiHeader.biYPelsPerMeter = 0;
    bmpfile.biInfo.bmiHeader.biClrUsed = 0;
    bmpfile.biInfo.bmiHeader.biClrImportant = 0;
    fwrite(&(bmpfile.bfHeader), sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&(bmpfile.biInfo.bmiHeader), sizeof(BITMAPINFOHEADER), 1, fp);
    U8 *pEachLinBuf = (U8*)malloc(bmppitch);
    memset(pEachLinBuf, 0, bmppitch);
    U8 BytePerPix = bitCountPerPix >> 3;
    U32 pitch = width * BytePerPix;
    if(pEachLinBuf){
        int h,w;
        for(h = height-1; h >= 0; h--){
            for(w = 0; w < width; w++){
                //copy by a pixel
                pEachLinBuf[w*BytePerPix+0] = pData[h*pitch + w*BytePerPix + 2];
                pEachLinBuf[w*BytePerPix+1] = pData[h*pitch + w*BytePerPix + 1];
                pEachLinBuf[w*BytePerPix+2] = pData[h*pitch + w*BytePerPix + 0];
            }
            fwrite(pEachLinBuf, bmppitch, 1, fp);
        }
        free(pEachLinBuf);
    }
    fclose(fp);
    return 1;
}

//获取BMP文件的位图数据(无颜色表的位图):丢掉BMP文件的文件信息头和位图信息头，获取其RGB(A)位图数据
U8* GetFileBmpData(U8 *bitCountPerPix, U32 *width, U32 *height, const char* filename){
    FILE *pf = fopen(filename, "rb");
    if(!pf){
        printf("fopen failed : %s, %d\n", __FILE__, __LINE__);
        return NULL;
    }
    BITMAPFILE bmpfile;
    fread(&(bmpfile.bfHeader), sizeof(BITMAPFILEHEADER), 1, pf);
    fread(&(bmpfile.biInfo.bmiHeader), sizeof(BITMAPINFOHEADER), 1, pf);
   // print_bmfh(bmpfile.bfHeader);
   // print_bmih(bmpfile.biInfo.bmiHeader);
    if(bitCountPerPix){
        *bitCountPerPix = bmpfile.biInfo.bmiHeader.biBitCount;
    }
    if(width){
        *width = bmpfile.biInfo.bmiHeader.biWidth;
    }
    if(height){
        *height = bmpfile.biInfo.bmiHeader.biHeight;
    }
    U32 bmppicth = (((*width)*(*bitCountPerPix) + 31) >> 5) << 2;
    U8 *pdata = (U8*)malloc((*height)*bmppicth);
    U8 *pEachLinBuf = (U8*)malloc(bmppicth);
    memset(pEachLinBuf, 0, bmppicth);
    U8 BytePerPix = (*bitCountPerPix) >> 3;
    U32 pitch = (*width) * BytePerPix;
    if(pdata && pEachLinBuf){
        int w, h;
        for(h = (*height) - 1; h >= 0; h--){
            fread(pEachLinBuf, bmppicth, 1, pf);
            for(w = 0; w < (*width); w++){
                pdata[h*pitch + w*BytePerPix + 0] = pEachLinBuf[w*BytePerPix+0];
                pdata[h*pitch + w*BytePerPix + 1] = pEachLinBuf[w*BytePerPix+1];
                pdata[h*pitch + w*BytePerPix + 2] = pEachLinBuf[w*BytePerPix+2];
            }
        }
        free(pEachLinBuf);
    }
    fclose(pf);
    return pdata;
}

//获取BMP文件的位图数据(无颜色表的位图):丢掉BMP文件的文件信息头和位图信息头，获取其RGB(A)位图数据
U8* GetBmpData(U8 *bitCountPerPix, U32 *width, U32 *height,U8* bitmapData){
	BITMAPFILE bmpfile;
	auto headerSize = sizeof(BITMAPFILEHEADER);
	memcpy(&(bmpfile.bfHeader), bitmapData, headerSize);
	bitmapData += headerSize;
	headerSize = sizeof(BITMAPINFOHEADER);
	memcpy(&(bmpfile.biInfo.bmiHeader), bitmapData, headerSize);
	bitmapData += headerSize;
	if (bitCountPerPix){
		*bitCountPerPix = bmpfile.biInfo.bmiHeader.biBitCount;
	}
	if (width){
		*width = bmpfile.biInfo.bmiHeader.biWidth;
	}
	if (height){
		*height = bmpfile.biInfo.bmiHeader.biHeight;
	}
	U32 bmppicth = (((*width)*(*bitCountPerPix) + 31) >> 5) << 2;
	U8 *pdata = (U8*)malloc((*height)*bmppicth);
	U8 *pEachLinBuf = (U8*)malloc(bmppicth);
	memset(pEachLinBuf, 0, bmppicth);
	U8 BytePerPix = (*bitCountPerPix) >> 3;
	U32 pitch = (*width) * BytePerPix;
	if (pdata && pEachLinBuf){
		int w, h;
		for (h = (*height) - 1; h >= 0; h--){
			memcpy(pEachLinBuf, bitmapData, bmppicth);
			bitmapData += bmppicth;
			for (w = 0; w < (*width); w++){
				pdata[h*pitch + w*BytePerPix + 0] = pEachLinBuf[w*BytePerPix + 0];
				pdata[h*pitch + w*BytePerPix + 1] = pEachLinBuf[w*BytePerPix + 1];
				pdata[h*pitch + w*BytePerPix + 2] = pEachLinBuf[w*BytePerPix + 2];
			}
		}
		free(pEachLinBuf);
	}
	return pdata;
}


void GetBmpInfo (U8 *bitCountPerPix, U32 *width, U32 *height, U8* bitmapData)
{
   BITMAPFILE bmpfile;
   	auto headerSize = sizeof(BITMAPFILEHEADER);
   	memcpy(&(bmpfile.bfHeader), bitmapData, headerSize);
   	bitmapData += headerSize;
   	headerSize = sizeof(BITMAPINFOHEADER);
   	memcpy(&(bmpfile.biInfo.bmiHeader), bitmapData, headerSize);
   	bitmapData += headerSize;
   	if (bitCountPerPix){
   		*bitCountPerPix = bmpfile.biInfo.bmiHeader.biBitCount;
   	}
   	if (width){
   		*width = bmpfile.biInfo.bmiHeader.biWidth;
   	}
   	if (height){
   		*height = bmpfile.biInfo.bmiHeader.biHeight;
   	}

}