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
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
struct OpenSLStream {
    // engine interfaces
    SLObjectItf engineObject;
    SLEngineItf engineEngine;
    // output mix interfaces
    SLObjectItf outputMixObject;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb;
    // buffer queue player interfaces
    SLObjectItf bqPlayerObject;
    SLPlayItf bqPlayerPlay;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
    SLEffectSendItf bqPlayerEffectSend;
    SLMuteSoloItf bqPlayerMuteSolo;
    SLVolumeItf bqPlayerVolume;
    SLmilliHertz bqPlayerSampleRate;
    OpenSLStream(){
        // engine interfaces
        engineObject = nullptr;
        engineEngine = nullptr;
        // output mix interfaces
        outputMixObject = nullptr;
        outputMixEnvironmentalReverb = nullptr;
        // buffer queue player interfaces
        bqPlayerObject = nullptr;
        bqPlayerPlay = nullptr;
        bqPlayerBufferQueue = nullptr;
        SLEffectSendItf bqPlayerEffectSend = nullptr;
        SLMuteSoloItf bqPlayerMuteSolo = nullptr;
        SLVolumeItf bqPlayerVolume = nullptr;
        SLmilliHertz bqPlayerSampleRate = 0;
    }
};
enum {
    kMsgCodecBuffer,
    kMsgPause,
    kMsgResume,
    kMsgReStart,
    kMsgPauseAck,
    kMsgDecodeDone,
    kMsgSeek,
    kMsgLoadVideo,
    kMsgLoadAudio,
};
struct LooperMessage {
    int what;
    void *obj;
    LooperMessage *next;
    bool quit;
    LooperMessage(){
        what = kMsgPause;
        obj  = nullptr;
        next = nullptr;
        quit = false;
    }
};
struct WorkerData{
    int fd;
    ANativeWindow* _aNativeWindow;
    AMediaExtractor* _aMediaExtractor;
    AMediaCodec *_aMediaCodec;
    int64_t _start;
    int32_t _sampleRate;
    int32_t _channels;
    bool _sawInputEOS;
    bool _sawOutputEOS;
    bool _isPlaying;
    bool _playOnce;
    int64_t _videoDuration;
    int64_t _audioDuration;
    float _progress;
    WorkerData(){
        fd  =   -1;
        _aNativeWindow = nullptr;
        _aMediaExtractor = nullptr;
        _aMediaCodec = nullptr;
        _start = 0;
        _sampleRate = 44100;
        _channels = 1;
        _sawInputEOS = false;
        _sawOutputEOS = false;
        _isPlaying = false;
        _playOnce = false;
        _videoDuration = 0;
        _audioDuration = 0;
        _progress = 0.0f;
    }
};
class MediaCodec{
    public:
        MediaCodec();
        MediaCodec& operator=(const MediaCodec& ) = delete;
        MediaCodec(MediaCodec&) = delete;
        virtual ~MediaCodec();
        void post(int what, void *data, bool flush = false);
        virtual void quit() = 0;
        virtual void handle(LooperMessage* looperMessage) = 0;
        void nativeSetPlaying(bool play);
        void nativeShutDown();
        void nativeUpdateProgress(float progress);
        void nativeRestart();
    protected:
        virtual void doCodecWork(WorkerData *d) = 0;
        void addMsg(LooperMessage *msg, bool flush);
        LooperMessage *_headLooperMessage;
        bool running;
        void _mediaCodecLoop();
        std::thread _mediaCodecThread;
        sem_t _headWriteProtect;
        sem_t _headDataAvailable;
        WorkerData _workerData;
};
