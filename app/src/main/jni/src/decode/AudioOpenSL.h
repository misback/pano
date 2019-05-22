#pragma once
#include "Singleton.h"
#include "faac.h"
#include "mp4v2/mp4v2.h"
#include <vector>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <thread>
#include <condition_variable>
#include <mutex>
#define SAMPLE_RATE         44100
#define CHANNELS            1
#define PERIOD_TIME         40 //ms
#define FRAME_SIZE          SAMPLE_RATE*PERIOD_TIME/1000
#define BUFFER_SIZE         FRAME_SIZE*CHANNELS
#define AUDIO_SAMPLE_RATE   44100
#define AUDIO_DURATION      1024
#define PCM_BIT_SIZE        16
#define AUDIO_MAX_SIZE      90
struct AACAudio{
    enum class AACAudio_STATE{
        IDLEING     =   0,
        READING     =   1,
        WRITING     =   2,
        READABLE    =   3
    };
    short                       data[BUFFER_SIZE];
    volatile AACAudio_STATE    state;
    int                         size;
    volatile size_t            curTime;
    AACAudio(){
        state                   =   AACAudio_STATE::IDLEING;
        size                    =   0;
        curTime                 =   0;
    }
    void clear(){
        state                   =   AACAudio_STATE::IDLEING;
        size                    =   0;
        curTime                 =   0;
    }
};

class AudioOpenSL:public Singleton<AudioOpenSL>{
public:
    AudioOpenSL();
    ~AudioOpenSL();
private:
    // engine interfaces
    SLObjectItf                     _engineObject;
    SLEngineItf                     _engineEngine;
    // recorder interfaces
    SLObjectItf                     _recorderObject;
    SLRecordItf                     _recorderRecord;
    SLAndroidSimpleBufferQueueItf   _recorderBufferQueue;

    faacEncHandle                   _hEncoder;
    unsigned int                  _nPCMBitSize;      // 单样本位数
    unsigned long                 _nInputSamples;
    unsigned long                 _nMaxOutputBytes;

    std::vector<AACAudio*>          _dataVec;
    int                             _readPos;
    int                             _writePos;
    std::mutex                      _mutex;
    std::condition_variable         _condVar;
    std::thread                     _audioRecorderThread;
    int                             _conditionVar;
    volatile int                   _exitAudioRecorder;
    int                             _currentInputIndex;
    int                             _currentInputBuffer;
    short*                          _audioInputBuffer[2];
    volatile size_t                _preDurationTime;
    volatile size_t                _curDurationTime;
    unsigned long                 _skipFrameNum;

    void _audioRecorderThreadFunc();
    bool android_OpenAudioDevice();
    void android_CloseAudioDevice();
    int android_AudioIn(short *buffer,int size);
	void clearAudio(){
	    auto size   =   _dataVec.size();
	    for(auto i=0; i<size; i++){
	        if(_dataVec[i] != nullptr){
	            _dataVec[i]->clear();
	        }
	    }
	    memset(_audioInputBuffer, 0, sizeof(_audioInputBuffer));
	}
	AACAudio* getWrite(){
	    AACAudio* writeData =   nullptr;
        if(_dataVec[_writePos] != nullptr && _dataVec[_writePos]->state == AACAudio::AACAudio_STATE::READING){
            _writePos   =  (_writePos+1)%AUDIO_MAX_SIZE;
        }
        writeData       =   _dataVec[_writePos];
        _writePos       =  (_writePos+1)%AUDIO_MAX_SIZE;
        return writeData;
	}
    int getReadElementNum(){
        int num =   0;
        for(auto element:_dataVec){
            if(element != nullptr && element->state == AACAudio::AACAudio_STATE::READABLE){
                num ++;
            }
        }
        return num;
    }
    std::vector<AACAudio*> getRead(){
        std::vector<AACAudio*> aacAudioVec;
        auto readNum =   getReadElementNum();
        if(readNum > 0){
            for(int i=_readPos; i<(_readPos+AUDIO_MAX_SIZE); i++){
                auto aacAudio   =   _dataVec[i%AUDIO_MAX_SIZE];
                if (aacAudio->state  ==  AACAudio::AACAudio_STATE::READABLE){
                    aacAudioVec.push_back(aacAudio);
                }
            }
        }
        return aacAudioVec;
    }
    void recorderCallBack();
public:
    static void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
    void writeAACAudioOpenSL(MP4FileHandle _hMP4FileHandle, MP4TrackId _audioTrackId, size_t duration, int isKey);
    int startRecording();
    void stop();
};
