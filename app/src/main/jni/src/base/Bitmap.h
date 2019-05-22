//
// Created by roading on 2016/6/24.
//

#ifndef NANO_NANO_BITMAP_H
#define NANO_NANO_BITMAP_H
#pragma pack(push, 1)
typedef unsigned char  U8;
typedef unsigned short U16;
typedef unsigned int   U32;
typedef struct tagBITMAPFILEHEADER{
    U16 bfType;
    U32 bfSize;
    U16 bfReserved1;
    U16 bfReserved2;
    U32 bfOffBits;
} BITMAPFILEHEADER;
typedef struct tagBITMAPINFOHEADER{
    U32 biSize;
    U32 biWidth;
    U32 biHeight;
    U16 biPlanes;
    U16 biBitCount;
    U32 biCompression;
    U32 biSizeImage;
    U32 biXPelsPerMeter;
    U32 biYPelsPerMeter;
    U32 biClrUsed;
    U32 biClrImportant;
} BITMAPINFOHEADER;
typedef struct tagRGBQUAD{
    U8 rgbBlue;
    U8 rgbGreen;
    U8 rgbRed;
    U8 rgbReserved;
} RGBQUAD;
typedef struct tagBITMAPINFO{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
} BITMAPINFO;
typedef struct tagBITMAP{
    BITMAPFILEHEADER bfHeader;
    BITMAPINFO biInfo;
}BITMAPFILE;
extern int GenBmpFile(U8 *pData, U32 width, U32 height, const char *filename);
extern int GenBmpFile(U8 *pData, U8 bitCountPerPix, U32 width, U32 height, const char *filename);
extern int GenBmpFile_Gray(U8 *pData, U8 bitCountPerPix, U32 width, U32 height, const char *filename);
extern int GenBmpFile_BGR(U8 *pData, U8 bitCountPerPix, U32 width, U32 height, const char *filename);
extern U8* GetFileBmpData(U8 *bitCountPerPix, U32 *width, U32 *height, const char* filename);
extern U8* GetBmpData(U8 *bitCountPerPix, U32 *width, U32 *height, U8* bitmapData);

extern void GetBmpInfo (U8 *bitCountPerPix, U32 *width, U32 *height, U8* bitmapData);

#pragma pack(pop)
#endif //NANO_NANO_BITMAP_H
