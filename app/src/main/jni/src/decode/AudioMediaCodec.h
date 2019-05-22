//
// Created by DELL on 2017/3/8.
//
#pragma once
#include "MediaCodec.h"
// for native audio
class AudioInterface{
public:
    virtual void callback(uint8_t* data, ssize_t size) = 0;
};
class AudioMediaCodec:public MediaCodec{
    public:
        AudioMediaCodec(AudioInterface* audioInterface);
        AudioMediaCodec& operator=(const AudioMediaCodec& ) = delete;
        AudioMediaCodec(AudioMediaCodec&) = delete;
        virtual ~AudioMediaCodec();
        void quit();
        void handle(LooperMessage* looperMessage);
        void loadSource(AMediaExtractor* aMediaExtractor);
    private:
        void doCodecWork(WorkerData *d);
        sem_t _pcmDataAvailable;
        int _signedStart;
        OpenSLStream openSLStream;
        AudioInterface* _audioInterface;
    private:
        void createOpenSL(int sampleRate, int channels);
        void destroyOpenSL();
    public:
        void bqPlayerCallback();
        void nativeSetSaveFilter(int saveFilter);
};
