//
// Created by DELL on 2017/3/8.
//
#include "VideoMediaCodec.h"
#include <assert.h>
#include <jni.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <semaphore.h>
#include "Common.h"
#include "JniHelper.h"
VideoMediaCodec::VideoMediaCodec(){
    _onProgressCallbackObj = nullptr;
    _onProgress = nullptr;
}
VideoMediaCodec::~VideoMediaCodec() {
    if (running) {
        LOGE("Looper deleted while still running. Some messages will not be processed");
        quit();
    }
}
void VideoMediaCodec::quit() {
    LooperMessage *msg = new LooperMessage();
    msg->what = kMsgDecodeDone;
    msg->obj = &_workerData;
    msg->next = nullptr;
    msg->quit = true;
    addMsg(msg, true);
    _mediaCodecThread.join();
    sem_destroy(&_headDataAvailable);
    sem_destroy(&_headWriteProtect);
    running = false;
    if(_workerData._aMediaCodec!=nullptr){
        AMediaCodec_stop(_workerData._aMediaCodec);
        AMediaCodec_delete(_workerData._aMediaCodec);
        _workerData._aMediaCodec = nullptr;
    }
    if (_workerData._aNativeWindow != nullptr) {
        ANativeWindow_release(_workerData._aNativeWindow);
        _workerData._aNativeWindow = nullptr;
        LOGE("QUIT");
    }
}
void VideoMediaCodec::doCodecWork(WorkerData *d) {
    if(!d->_sawInputEOS){
        auto bufferId = AMediaCodec_dequeueInputBuffer(d->_aMediaCodec, 0);
        if (bufferId >= 0) {
            size_t bufferSize;
            auto buf = AMediaCodec_getInputBuffer(d->_aMediaCodec, bufferId, &bufferSize);
            auto sampleSize = AMediaExtractor_readSampleData(d->_aMediaExtractor, buf, bufferSize);
            auto presentationTimeUs = AMediaExtractor_getSampleTime(d->_aMediaExtractor);
            callback(presentationTimeUs);
            if (sampleSize < 0) {
                d->_sawInputEOS = true;
                AMediaCodec_queueInputBuffer(d->_aMediaCodec, bufferId, 0, 0, presentationTimeUs, AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
            }else{
                AMediaCodec_queueInputBuffer(d->_aMediaCodec, bufferId, 0, sampleSize, presentationTimeUs, 0);
                AMediaExtractor_advance(d->_aMediaExtractor);
            }
        }
    }
    if(!d->_sawOutputEOS){
        AMediaCodecBufferInfo info;
        auto status = AMediaCodec_dequeueOutputBuffer(d->_aMediaCodec, &info, 0);
        if (status >= 0) {
            if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
                d->_sawOutputEOS = true;
            }
            int64_t presentationNano = info.presentationTimeUs * 1000;
            if (d->_start < 0) {
                d->_start = systemnanotime() - presentationNano;
            }
            int64_t delay = (d->_start + presentationNano) - systemnanotime();
            if (delay > 0) {
                usleep(delay / 1000);
            }
            AMediaCodec_releaseOutputBuffer(d->_aMediaCodec, status, info.size != 0);
            if (d->_playOnce) {
                d->_playOnce = false;
                return;
            }
        } else if (status == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
            LOGV("output buffers changed");
        } else if (status == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            auto format = AMediaCodec_getOutputFormat(d->_aMediaCodec);
            LOGV("format changed to: %s", AMediaFormat_toString(format));
            AMediaFormat_delete(format);
        } else if (status == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            LOGV("no output buffer right now");
        } else {
            LOGV("unexpected info code: %zd", status);
        }
    }
    if (!d->_sawInputEOS || !d->_sawOutputEOS) {
        post(kMsgCodecBuffer, d);
    }
}
void VideoMediaCodec::callback(int64_t duration){
    JNIEnv *env             =   JniHelper::getEnv();
    if (_onProgressCallbackObj != nullptr) {
        env->CallVoidMethod(_onProgressCallbackObj, _onProgress, duration);
        env->ExceptionClear();
    }
}
void VideoMediaCodec::setOnProgressCallback(jobject onProgress_callback_obj, jmethodID onProgress){
    _onProgressCallbackObj = onProgress_callback_obj;
    _onProgress = onProgress;
}
void VideoMediaCodec::loadSource(AMediaExtractor* aMediaExtractor, ANativeWindow* window){
    WorkerData* workerData = new WorkerData();
    *workerData = _workerData;
    workerData->_aMediaExtractor = aMediaExtractor;
    workerData->_aNativeWindow = window;
    post(kMsgLoadVideo, workerData);
}
void VideoMediaCodec::handle(LooperMessage* looperMessage) {
    auto what = looperMessage->what;
    auto obj = looperMessage->obj;
    switch (what) {
        case kMsgCodecBuffer:{
            doCodecWork((WorkerData*)obj);
        }
            break;
        case kMsgDecodeDone: {
            WorkerData *d = (WorkerData*)obj;
            if(d->_aMediaCodec!=nullptr){
                AMediaCodec_stop(d->_aMediaCodec);
                AMediaCodec_delete(d->_aMediaCodec);
                d->_aMediaCodec = nullptr;
            }
            if(d->_aMediaExtractor!=nullptr){
                AMediaExtractor_delete(d->_aMediaExtractor);
                d->_aMediaExtractor = nullptr;
            }
            d->_sawInputEOS = true;
            d->_sawOutputEOS = true;
        }
        break;
        case kMsgSeek: {
            WorkerData *d = (WorkerData*)obj;
            if(d->_aMediaExtractor!=nullptr && d->_aMediaCodec!=nullptr){
                AMediaExtractor_seekTo(d->_aMediaExtractor, d->_progress*d->_videoDuration, AMEDIAEXTRACTOR_SEEK_NEXT_SYNC);
                AMediaCodec_flush(d->_aMediaCodec);
                d->_start = -1;
                d->_sawInputEOS = false;
                d->_sawOutputEOS = false;
                if (!d->_isPlaying) {
                    d->_playOnce = true;
                    post(kMsgCodecBuffer, d);
                }
            }
        }
        break;
        case kMsgPause: {
            WorkerData *d = (WorkerData*)obj;
            if (d->_isPlaying) {
                d->_isPlaying = false;
                post(kMsgPauseAck, nullptr, true);
            }
        }
        break;
        case kMsgResume: {
            WorkerData *d = (WorkerData*)obj;
            if(d->_aMediaExtractor == nullptr || d->_aMediaCodec == nullptr){
                post(kMsgDecodeDone, d);
                return;
            }
            if(d->_sawInputEOS && d->_sawOutputEOS){
                AMediaExtractor_seekTo(d->_aMediaExtractor, 0, AMEDIAEXTRACTOR_SEEK_PREVIOUS_SYNC);
                AMediaCodec_flush(d->_aMediaCodec);
                d->_start = -1;
                d->_sawInputEOS = false;
                d->_sawOutputEOS = false;
                post(kMsgCodecBuffer, d);
            }else if (!d->_isPlaying) {
                d->_start = -1;
                d->_isPlaying = true;
                post(kMsgCodecBuffer, d);
            }
        }
        break;
        case kMsgReStart: {
            WorkerData *d = (WorkerData*)obj;
            if(d->_aMediaExtractor == nullptr || d->_aMediaCodec == nullptr || !running){
                post(kMsgDecodeDone, d);
                return;
            }
            AMediaExtractor_seekTo(d->_aMediaExtractor, 0, AMEDIAEXTRACTOR_SEEK_PREVIOUS_SYNC);
            AMediaCodec_flush(d->_aMediaCodec);
            d->_start = -1;
            d->_sawInputEOS = false;
            d->_sawOutputEOS = false;
            d->_isPlaying = false;
            post(kMsgCodecBuffer, d);
        }
        break;

        case kMsgLoadVideo:{
            if(_workerData._aMediaCodec != nullptr){
                AMediaCodec_stop(_workerData._aMediaCodec);
                AMediaCodec_delete(_workerData._aMediaCodec);
                _workerData._aMediaCodec = nullptr;
            }
            if(_workerData._aMediaExtractor != nullptr){
                AMediaExtractor_delete(_workerData._aMediaExtractor);
                _workerData._aMediaExtractor = nullptr;
            }
            if (_workerData._aNativeWindow != nullptr) {
                ANativeWindow_release(_workerData._aNativeWindow);
                _workerData._aNativeWindow = nullptr;
            }
            WorkerData* newWorkerData = (WorkerData*)obj;
            _workerData._aMediaExtractor = newWorkerData->_aMediaExtractor;
            _workerData._aNativeWindow = newWorkerData->_aNativeWindow;
            delete newWorkerData;
            looperMessage->obj = nullptr;
            auto numTracks = AMediaExtractor_getTrackCount(_workerData._aMediaExtractor);
            LOGE("input has %d tracks", numTracks);
            for (int i = 0; i < numTracks; i++) {
                auto format = AMediaExtractor_getTrackFormat(_workerData._aMediaExtractor, i);
                auto s = AMediaFormat_toString(format);
                LOGE("track %d format: %s", i, s);
                const char *mime;
                if (!AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime)) {
                    LOGE("no mime type");
                    AMediaFormat_delete(format);
                    break;
                }
                if (strncmp(mime, "video/", 6) == 0) {
                    LOGE("[[[[[[[[[[[%s", mime);
                    AMediaExtractor_selectTrack(_workerData._aMediaExtractor, i);
                    auto codecVideo = AMediaCodec_createDecoderByType(mime);
                    //int width = 0;
                    //int height = 0;
                    //AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &width);
                    //AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &height);
                    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, 480);
                    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, 320);
                    AMediaCodec_configure(codecVideo, format, _workerData._aNativeWindow, nullptr, 0);
                    AMediaFormat_getInt64(format, AMEDIAFORMAT_KEY_DURATION, &(_workerData._videoDuration));
                    AMediaFormat_delete(format);
                    _workerData._aMediaCodec = codecVideo;
                    _workerData._start = -1;
                    _workerData._sawInputEOS = false;
                    _workerData._sawOutputEOS = false;
                    _workerData._isPlaying = false;
                    _workerData._playOnce = true;
                    AMediaCodec_start(_workerData._aMediaCodec);
                    post(kMsgCodecBuffer, &_workerData, true);
                    post(kMsgResume, &_workerData);
                    break;
                }else{
                    AMediaFormat_delete(format);
                }
            }
        }
        break;
    }
}