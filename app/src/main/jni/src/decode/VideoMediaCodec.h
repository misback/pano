//
// Created by DELL on 2017/3/8.
//
#pragma once
#include "MediaCodec.h"
class VideoMediaCodec:public MediaCodec{
    public:
        VideoMediaCodec();
        VideoMediaCodec& operator=(const VideoMediaCodec& ) = delete;
        VideoMediaCodec(VideoMediaCodec&) = delete;
        virtual ~VideoMediaCodec();
        void quit();
        void handle(LooperMessage* looperMessage);
        void loadSource(AMediaExtractor* aMediaExtractor, ANativeWindow* window);
        void setOnProgressCallback(jobject onProgress_callback_obj, jmethodID onProgress);
    private:
        void doCodecWork(WorkerData *d);
        void callback(int64_t duration);
        jobject _onProgressCallbackObj;
        jmethodID _onProgress;
};
