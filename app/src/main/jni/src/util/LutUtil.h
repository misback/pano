#pragma once
#include "android/asset_manager.h"
#include <android/asset_manager_jni.h>
#include "Bitmap.h"
class LutUtil{
    private:
        LutUtil(){}
        ~LutUtil(){}
    public:
        static U8* loadLutFile(AAssetManager* mgr, const char* filename, int* lutSize);
        static U8* loadLutFileF(AAssetManager* mgr, const char* filename, int* lutSize);
        static U8* loadLutFileB(AAssetManager* mgr, const char* filename, int* lutSize);
};
