//
// Created by DELL on 2017/3/8.
//
#include "CameraMediaCodec.h"
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
#define MIME_TYPE "video/avc"
#define H264_FRAME_P_4                                          0x09
#define H264_FRAME_P_5                                          0x30
CameraMediaCodec::CameraMediaCodec():
_headCameraMediaCodecMsg(nullptr),
_uvcIsNeedDecode(false),
_isHadIFrame(false),
_connectState(false)
{
    sem_init(&_headDataAvailable, 0, 0);
    sem_init(&_headWriteProtect, 0, 1);
    running = true;
    _mediaCodecThread          =   std::thread(&CameraMediaCodec::_mediaCodecLoop, this);
}
CameraMediaCodec::~CameraMediaCodec() {
    if (running) {
        LOGE("Looper deleted while still running. Some messages will not be processed");
        quit();
    }
    clear(false);
}
void CameraMediaCodec::quit() {
    CameraMediaCodecMsg *msg = new CameraMediaCodecMsg();
    msg->what = CAMERA_MEDIA_CODEC_MSG_TYPE::kMsgDecodeDone;
    msg->obj = &_cameraMediaCodecData;
    msg->next = nullptr;
    msg->quit = true;
    addMsg(msg, true);
    _mediaCodecThread.join();
    sem_destroy(&_headDataAvailable);
    sem_destroy(&_headWriteProtect);
    running = false;
    if(_cameraMediaCodecData._aMediaCodec!=nullptr){
        AMediaCodec_stop(_cameraMediaCodecData._aMediaCodec);
        AMediaCodec_delete(_cameraMediaCodecData._aMediaCodec);
        _cameraMediaCodecData._aMediaCodec = nullptr;
    }
    if (_cameraMediaCodecData._aNativeWindow != nullptr) {
        ANativeWindow_release(_cameraMediaCodecData._aNativeWindow);
        _cameraMediaCodecData._aNativeWindow = nullptr;
        LOGE("QUIT");
    }
}
void CameraMediaCodec::post(CAMERA_MEDIA_CODEC_MSG_TYPE what, CameraMediaCodecData *cameraMediaCodecData, bool flush) {
    CameraMediaCodecMsg *msg = new CameraMediaCodecMsg();
    msg->what = what;
    msg->obj = cameraMediaCodecData;
    msg->next = nullptr;
    msg->quit = false;
    addMsg(msg, flush);
}
void CameraMediaCodec::post(CAMERA_MEDIA_CODEC_MSG_TYPE what, bool flush) {
    CameraMediaCodecMsg *msg = new CameraMediaCodecMsg();
    msg->what = what;
    msg->obj = &_cameraMediaCodecData;
    msg->next = nullptr;
    msg->quit = false;
    addMsg(msg, flush);
}
void CameraMediaCodec::addMsg(CameraMediaCodecMsg *msg, bool flush) {
    sem_wait(&_headWriteProtect);
    CameraMediaCodecMsg *h = _headCameraMediaCodecMsg;
    if (flush) {
        while(h) {
            CameraMediaCodecMsg *next = h->next;
            delete h;
            h = next;
        }
        h = nullptr;
    }
    if (h) {
        while (h->next) {
            h = h->next;
        }
        h->next = msg;
    } else {
        _headCameraMediaCodecMsg = msg;
    }
    sem_post(&_headWriteProtect);
    sem_post(&_headDataAvailable);
}
void CameraMediaCodec::_mediaCodecLoop() {
    while(true) {
        // wait for available message
        sem_wait(&_headDataAvailable);
        // get next available message
        sem_wait(&_headWriteProtect);
        CameraMediaCodecMsg *msg = _headCameraMediaCodecMsg;
        if (msg == nullptr) {
            sem_post(&_headWriteProtect);
            continue;
        }
        _headCameraMediaCodecMsg = msg->next;
        sem_post(&_headWriteProtect);

        if (msg->quit) {
            LOGE("quitting");
            delete msg;
            return;
        }
        handle(msg);
        delete msg;
    }
}
void CameraMediaCodec::doCodecWork(CameraMediaCodecData *d) {
    if(!d->_sawInputEOS){
        auto readFrame           =   _readFrame();
        if(readFrame != nullptr && _isHadIFrame){
            auto frame           =   readFrame->data;
            if (frame != nullptr && d->_aMediaCodec != nullptr) {
                ssize_t index = 0, size = 0;
                while (index < readFrame->size){
                    auto bufferId = AMediaCodec_dequeueInputBuffer(d->_aMediaCodec, 0);
                    if (bufferId >= 0) {
                        size_t bufferSize;
                        auto buf = AMediaCodec_getInputBuffer(d->_aMediaCodec, bufferId, &bufferSize);
                        auto restSize = readFrame->size - index;
                        size = (bufferSize <= restSize ? bufferSize : restSize);
                        memcpy(buf, frame+index, size);
                        index += size;
                        auto presentationTimeUs = systemnanotime();
                        if (size <= 0) {
                            AMediaCodec_queueInputBuffer(d->_aMediaCodec, bufferId, 0, 0, presentationTimeUs, AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
                        }else{
                            AMediaCodec_queueInputBuffer(d->_aMediaCodec, bufferId, 0, size, presentationTimeUs, 0);
                        }
                        AMediaCodecBufferInfo info;
                        auto status = AMediaCodec_dequeueOutputBuffer(d->_aMediaCodec, &info, 0);
                        while (status >= 0) {
                            AMediaCodec_releaseOutputBuffer(d->_aMediaCodec, status, info.size != 0);
                            status = AMediaCodec_dequeueOutputBuffer(d->_aMediaCodec, &info, 0);
                        }
                    }else{
                        break;
                    }
                }
            }
        }
        if(readFrame != nullptr){
            delete readFrame;
        }
    }
}
void CameraMediaCodec::handle(CameraMediaCodecMsg* looperMessage) {
    auto what = looperMessage->what;
    auto obj = looperMessage->obj;
    switch (what) {
        case CAMERA_MEDIA_CODEC_MSG_TYPE::kMsgCodecBuffer:{
            doCodecWork((CameraMediaCodecData*)obj);
        }
        break;
        case CAMERA_MEDIA_CODEC_MSG_TYPE::kMsgInitGL:{
            clear(false);
        }
        break;
        case CAMERA_MEDIA_CODEC_MSG_TYPE::kMsgConnect:{
            _connectState = true;
        }
        break;
        case CAMERA_MEDIA_CODEC_MSG_TYPE::kMsgDisConnect:{
            clear(false);
            _connectState = false;
        }
        break;
        case CAMERA_MEDIA_CODEC_MSG_TYPE::kMsgPause:{

        }
        break;
        case CAMERA_MEDIA_CODEC_MSG_TYPE::kMsgResume:{

        }
        break;
        case CAMERA_MEDIA_CODEC_MSG_TYPE::kMsgDecodeDone:{
            CameraMediaCodecData *d = (CameraMediaCodecData*)obj;
            if(d->_aMediaCodec!=nullptr){
                AMediaCodec_stop(d->_aMediaCodec);
                AMediaCodec_delete(d->_aMediaCodec);
                d->_aMediaCodec = nullptr;
            }
            d->_sawInputEOS = true;
        }
        break;
        case CAMERA_MEDIA_CODEC_MSG_TYPE::kMsgUpdateSurface:{
            if(_cameraMediaCodecData._aMediaCodec != nullptr){
                AMediaCodec_stop(_cameraMediaCodecData._aMediaCodec);
                AMediaCodec_delete(_cameraMediaCodecData._aMediaCodec);
                _cameraMediaCodecData._aMediaCodec = nullptr;
            }
            if (_cameraMediaCodecData._aNativeWindow) {
                ANativeWindow_release(_cameraMediaCodecData._aNativeWindow);
                _cameraMediaCodecData._aNativeWindow = nullptr;
            }
            CameraMediaCodecData* newWorkerData = (CameraMediaCodecData*)obj;
            _cameraMediaCodecData._aNativeWindow = newWorkerData->_aNativeWindow;
            _cameraMediaCodecData._aMediaCodec = AMediaCodec_createDecoderByType(MIME_TYPE);
            auto format = AMediaFormat_new();
            AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, MIME_TYPE);
            AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, 1920);
            AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, 960);
            media_status_t status = AMediaCodec_configure(_cameraMediaCodecData._aMediaCodec, format, _cameraMediaCodecData._aNativeWindow, nullptr, 0);
            status = AMediaCodec_start(_cameraMediaCodecData._aMediaCodec);
            AMediaFormat_delete(format);
        }
        break;
    }
}
bool CameraMediaCodec::isHadIFrame(){
    return _isHadIFrame;
}
void CameraMediaCodec::updateNeedDecode(bool uvcIsNeedDecode){
    _uvcIsNeedDecode = uvcIsNeedDecode;
}
Frame* CameraMediaCodec::_readFrame(){
    std::lock_guard<std::mutex> lk(mut);
    if(data_queue.size()>0){
        auto frame = data_queue.front();
        data_queue.pop();
        return frame;
    }else{
        return nullptr;
    }
}
void CameraMediaCodec::clear(bool isHadIFrame){
    _isHadIFrame = isHadIFrame;
    std::lock_guard<std::mutex> lk(mut);
    auto size = data_queue.size();
    for(auto i=0; i<size; i++){
        auto frame = data_queue.front();
        if(frame != nullptr){
            delete frame;
        }
        data_queue.pop();
    }
}
void CameraMediaCodec::_addFrame(Frame *frame){
    std::lock_guard<std::mutex> lk(mut);
    auto size = data_queue.size();
    auto step = 0;
    if(size>30){
        step = 3;
    }else if(size>20){
        step = 2;
    }
    for(auto i=0; i<step; i++){
        auto frame = data_queue.front();
        if(frame != nullptr){
            delete frame;
        }
        data_queue.pop();
    }
    data_queue.push(frame);
    post(CAMERA_MEDIA_CODEC_MSG_TYPE::kMsgCodecBuffer, false);
}
void CameraMediaCodec::_firstGetIFrame(unsigned char *buffer, size_t size){
    if(!_isHadIFrame){
        clear(true);
        _cameraMediaCodecData._sawInputEOS = false;
    }
}
void CameraMediaCodec::_onFrameCallback(unsigned char *buffer, unsigned int size, video_info_t info, void *vPtr_args) {
    CameraMediaCodec *cameraMediaCodec = reinterpret_cast<CameraMediaCodec *>(vPtr_args);
    if (cameraMediaCodec != nullptr && cameraMediaCodec->running && buffer != nullptr && info.format == VID_FORMAT_H264_RAW && size > 0) {
        bool isKeyFrame = false;
        if ((buffer[4] == H264_FRAME_P_4 && buffer[5] == H264_FRAME_P_5)) {
            isKeyFrame = false;
            if (!cameraMediaCodec->_isHadIFrame) {
                return;
            }
        } else {
            isKeyFrame = true;
            cameraMediaCodec->_firstGetIFrame(buffer, size);
        }
        unsigned char* cpBuffer   =   new unsigned char[size];
        memcpy(cpBuffer, buffer, size);
        Frame* frameElement =   new Frame();
        frameElement->data =   cpBuffer;
        frameElement->iskey =   isKeyFrame;
        frameElement->size =   size;
        cameraMediaCodec->_addFrame(frameElement);
    }
}