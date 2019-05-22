//
// Created by DELL on 2017/3/8.
//
#pragma once
#include "media/NdkMediaCodec.h"
#include "media/NdkMediaExtractor.h"
#include <android/log.h>
#include <android/native_window_jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <semaphore.h>
#include <thread>
#include <atomic>
#include "mxuvc.h"
#include <queue>
#include <mutex>
enum class CAMERA_MEDIA_CODEC_MSG_TYPE{
    kMsgCodecBuffer,
    kMsgInitGL,
    kMsgConnect,
    kMsgDisConnect,
    kMsgPause,
    kMsgResume,
    kMsgDecodeDone,
    kMsgUpdateSurface,
};
struct Frame{
    unsigned char*             data;
    size_t                       size;
    bool                        iskey;
    Frame(){
        iskey  =   false;
        data   =   nullptr;
        size   =   0;
    }
    ~Frame(){
        if(data != nullptr){
            delete[] data;
            data = nullptr;
        }
        iskey = false;
        size  = 0;
    }
};
struct CameraMediaCodecMsg {
    CAMERA_MEDIA_CODEC_MSG_TYPE what;
    void *obj;
    CameraMediaCodecMsg *next;
    bool quit;
    CameraMediaCodecMsg(){
        what = CAMERA_MEDIA_CODEC_MSG_TYPE::kMsgPause;
        obj  = nullptr;
        next = nullptr;
        quit = false;
    }
};
struct CameraMediaCodecData{
    ANativeWindow* _aNativeWindow;
    AMediaCodec *_aMediaCodec;
    bool _sawInputEOS;
    int _recordWidth;
    int _recordHeight;
    CameraMediaCodecData(){
        _aNativeWindow = nullptr;
        _aMediaCodec = nullptr;
        _sawInputEOS = false;
        _recordWidth = 0;
        _recordHeight = 0;
    }
};

class CameraMediaCodec{
    public:
        CameraMediaCodec();
        CameraMediaCodec& operator=(const CameraMediaCodec& ) = delete;
        CameraMediaCodec(CameraMediaCodec&) = delete;
        virtual ~CameraMediaCodec();
        void quit();
        void handle(CameraMediaCodecMsg* cameraMediaCodecMsg);
        void post(CAMERA_MEDIA_CODEC_MSG_TYPE what, CameraMediaCodecData *_cameraMediaCodecData, bool flush = false);
        void post(CAMERA_MEDIA_CODEC_MSG_TYPE what, bool flush = false);
        //FrameArray* _frameArray;
        void clear(bool isHasIFrame);

        bool isHadIFrame();
        void updateNeedDecode(bool uvcIsNeedDecode);
        void addMsg(CameraMediaCodecMsg *msg, bool flush);
        static void _onFrameCallback(unsigned char *buffer, unsigned int size, video_info_t info, void *vPtr_args);
    private:
        Frame* _readFrame();
        void _addFrame(Frame *frame);
        void _firstGetIFrame(unsigned char *buffer, size_t size);
        void doCodecWork(CameraMediaCodecData *d);
        CameraMediaCodecMsg *_headCameraMediaCodecMsg;
        std::atomic<bool> _uvcIsNeedDecode;
        std::atomic<bool> _isHadIFrame;
        std::atomic<bool> running;
        bool _connectState;
        void _mediaCodecLoop();
        std::thread _mediaCodecThread;
        sem_t _headWriteProtect;
        sem_t _headDataAvailable;
        CameraMediaCodecData _cameraMediaCodecData;
        std::mutex mut;
        std::queue<Frame*> data_queue;  // 1
};
