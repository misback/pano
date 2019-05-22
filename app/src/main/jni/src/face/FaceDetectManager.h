//
// Created by DELL on 2017/3/11.
//
#pragma once
#include "Common.h"
#include "Singleton.h"
#include "android/asset_manager.h"
#include <android/asset_manager_jni.h>
#include <FaceDetection/include/common.h>
#include <FaceDetection/include/face_detection.h>
#include <FaceAlignment/include/face_alignment.h>
struct FaceDetectInfo{
    seeta::FaceInfo faceInfo;
    seeta::FacialLandmark vLandmark[5];
};
class FaceDetectManager:public Singleton<FaceDetectManager>{
    public:
        FaceDetectManager(const char* tDetectModelPath, const char* tAlignModelPath);
        FaceDetectManager& operator=(const FaceDetectManager& ) = delete;
        FaceDetectManager(FaceDetectManager&) = delete;
        virtual ~FaceDetectManager();
        seeta::FaceDetection* _faceDetection;
        seeta::FaceAlignment* _faceAlignment;
        std::vector<FaceDetectInfo> run(unsigned char * gray, int cols, int rows);
};
