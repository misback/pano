//
// Created by DELL on 2017/3/11.
//
#include "FaceDetectManager.h"
template<> FaceDetectManager* Singleton<FaceDetectManager>::msSingleton = nullptr;
FaceDetectManager::FaceDetectManager(const char* tDetectModelPath, const char* tAlignModelPath)
{
    _faceDetection = new seeta::FaceDetection(tDetectModelPath);
    _faceAlignment = new seeta::FaceAlignment(tAlignModelPath);
}
FaceDetectManager::~FaceDetectManager(){
}
std::vector<FaceDetectInfo> FaceDetectManager::run(unsigned char * gray, int cols, int rows){
    seeta::ImageData img_gray;
    img_gray.data = gray;
    img_gray.width = cols;
    img_gray.height = rows;
    img_gray.num_channels = 1;
    std::vector<seeta::FaceInfo> faces = _faceDetection->Detect(img_gray);
    std::vector<FaceDetectInfo> vFaceDetectInfo;
    for(auto face:faces){
        FaceDetectInfo faceDetectInfo;
        faceDetectInfo.faceInfo = face;
        _faceAlignment->PointDetectLandmarks(img_gray, face, faceDetectInfo.vLandmark);
        vFaceDetectInfo.push_back(faceDetectInfo);
    }
    return vFaceDetectInfo;
}