#include "LutUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include "Common.h"
#include <math.h>
#include <strstream>
#include <string.h>
#include <malloc.h>
using namespace std;
U8* LutUtil::loadLutFile(AAssetManager* mgr, const char* filePath, int* lutSize){
     AAsset* asset 							= AAssetManager_open(mgr, filePath, AASSET_MODE_UNKNOWN);
     if(asset != nullptr){
         off_t fileSize 						= AAsset_getLength(asset);
         U8* lutData 			                = new U8[fileSize];
         int bytesread 							= AAsset_read(asset, (void*)lutData, fileSize);
         AAsset_close(asset);
         U8	bitCountPerPix;
         U32 width;
         U32 height;
//         U8* outData = GetBmpData(&bitCountPerPix, &width, &height, lutData);
//         *lutSize =   height;
         GetBmpInfo (&bitCountPerPix, &width, &height, lutData);

         U8* outData = (U8 *) malloc (width * height * 3) ;
         memcpy (outData , lutData + 54, width * height * 3 - 54) ;

         *lutSize = height;

         delete[] lutData;
         return outData;
 	}
 	*lutSize =   0;
 	return nullptr;
 }
U8* LutUtil::loadLutFileF(AAssetManager* mgr, const char* filePath, int* lutSize){
    AAsset* asset 							    = AAssetManager_open(mgr, filePath, AASSET_MODE_UNKNOWN);
    if(asset != nullptr){
        off_t fileSize 							= AAsset_getLength(asset);
        float* lutData 			                = new float[fileSize];
        int bytesread 							= AAsset_read(asset, (void*)lutData, fileSize);
        AAsset_close(asset);
        *lutSize =   round(pow(bytesread/12, 1.0/3.0));
        return (U8*)lutData;
	}
	*lutSize =   0;
	return nullptr;
}
U8* LutUtil::loadLutFileB(AAssetManager* mgr, const char* filePath, int* lutSize){
    AAsset* asset 							    = AAssetManager_open(mgr, filePath, AASSET_MODE_UNKNOWN);
    if(asset != nullptr){
        off_t fileSize 							= AAsset_getLength(asset);
        U8* lutData 			                = new U8[fileSize];
        int bytesread 							= AAsset_read(asset, (void*)lutData, fileSize);
        AAsset_close(asset);
        *lutSize =   round(pow(bytesread/3.0, 1.0/3.0));
        return lutData;
	}
	*lutSize =   0;
	return nullptr;
}

