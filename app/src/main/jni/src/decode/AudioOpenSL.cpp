#include "AudioOpenSL.h"
#include "JniHelper.h"
#include "Common.h"
#define SKIP_FRAME_NUM  20

template<> AudioOpenSL* Singleton<AudioOpenSL>::msSingleton = nullptr;
AudioOpenSL::AudioOpenSL():
_nPCMBitSize(PCM_BIT_SIZE),
_nInputSamples(0),
_nMaxOutputBytes(0),
_readPos(0),
_writePos(0)
{
    _engineObject            =   nullptr;
    _engineEngine            =   nullptr;
    _exitAudioRecorder       =   1;
    _currentInputIndex       =   BUFFER_SIZE;
    _currentInputBuffer      =   0;
    _conditionVar            =   0;
    _audioInputBuffer[0]     =   nullptr;
    _audioInputBuffer[1]     =   nullptr;
    // recorder interfaces
    _recorderObject          =   nullptr;
    _recorderRecord          =   nullptr;
    _recorderBufferQueue     =   nullptr;
    _preDurationTime         =   0;
    _curDurationTime         =   0;
    _skipFrameNum            =   0;
    _dataVec.reserve(AUDIO_MAX_SIZE);
    _dataVec                 =   std::vector<AACAudio*>(AUDIO_MAX_SIZE, nullptr);
    auto vecSize             =   _dataVec.size();
    for(auto i=0; i<vecSize; i++){
        _dataVec[i]          =   new AACAudio();
    }
}

AudioOpenSL::~AudioOpenSL(){
    for(auto element:_dataVec){
        if(element != nullptr){
            delete element;
        }
    }
    if(_audioInputBuffer[0] != nullptr){
        delete _audioInputBuffer[0];
        _audioInputBuffer[0] =   nullptr;
    }
    if(_audioInputBuffer[1] != nullptr){
        delete _audioInputBuffer[1];
        _audioInputBuffer[1] =   nullptr;
    }
    _dataVec.clear();
    faacEncClose(_hEncoder);
    android_CloseAudioDevice();
    _audioRecorderThread.join();
    _exitAudioRecorder  =   1;
}
void AudioOpenSL::recorderCallBack(){
    _conditionVar    =   0;
    _condVar.notify_all();
}
// this callback handler is called every time a buffer finishes recording
void AudioOpenSL::bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context){
    auto audioOpenSL = (AudioOpenSL *) context;
    if(audioOpenSL!=nullptr){
        audioOpenSL->recorderCallBack();
    }
}
void AudioOpenSL::writeAACAudioOpenSL(MP4FileHandle _hMP4FileHandle, MP4TrackId _audioTrackId, size_t duration, int isKey){
    if(_exitAudioRecorder == 1){
        _preDurationTime =   0;
        return;
    }
    auto readBufferVec  =   getRead();
    for(auto aacAudioObj:readBufferVec){
        if(aacAudioObj->state == AACAudio::AACAudio_STATE::READABLE){
            aacAudioObj->state     =   AACAudio::AACAudio_STATE::READING;
            if(aacAudioObj->size>0 && aacAudioObj->curTime>0){
                size_t duration     =   MP4_INVALID_DURATION;
                if(_preDurationTime>0 && aacAudioObj->curTime>=_preDurationTime){
                    duration        =   (aacAudioObj->curTime - _preDurationTime)*AUDIO_SAMPLE_RATE/1000;
                    MP4WriteSample(_hMP4FileHandle, _audioTrackId,  ((uint8_t*)aacAudioObj->data)+7, aacAudioObj->size-7, duration, 0, isKey);
                }
                _preDurationTime     =   aacAudioObj->curTime;
            }
            aacAudioObj->size       =   0;
            aacAudioObj->curTime    =   0;
            aacAudioObj->state     =   AACAudio::AACAudio_STATE::IDLEING;
        }
    }
}

int AudioOpenSL::startRecording(){
    if(_exitAudioRecorder == 0){
        return 0;
    }
    _preDurationTime         =   0;
    _curDurationTime         =   0;
    clearAudio();
    _currentInputIndex       =   BUFFER_SIZE;
    _currentInputBuffer      =   0;
    _conditionVar            =   0;
    if(_audioInputBuffer[0] == nullptr){
        _audioInputBuffer[0] =   new short[BUFFER_SIZE];
    }
    if(_audioInputBuffer[1] == nullptr){
        _audioInputBuffer[1] =   new short[BUFFER_SIZE];
    }
    if(android_OpenAudioDevice()){
        _hEncoder = faacEncOpen(SAMPLE_RATE, CHANNELS, &_nInputSamples, &_nMaxOutputBytes);
        if(_hEncoder == nullptr){
            LOGE("[ERROR] Failed to call faacEncOpen()\n");
        }
        faacEncConfigurationPtr myFormat;
        myFormat = faacEncGetCurrentConfiguration(_hEncoder);
        myFormat->allowMidside      =   0;//useMidSide;
        myFormat->aacObjectType     =   LOW;//objectType;
        myFormat->inputFormat       =   FAAC_INPUT_16BIT;
        myFormat->outputFormat      =   1;
        myFormat->useLfe            =   0;
        myFormat->bandWidth         =   16000;
        faacEncSetConfiguration(_hEncoder, myFormat);
        _audioRecorderThread        =   std::thread(&AudioOpenSL::_audioRecorderThreadFunc, this);
    }else{
        android_CloseAudioDevice();
        return 0;
    }
    return 1;
}

void AudioOpenSL::_audioRecorderThreadFunc(){
    JavaVM *vm              =   JniHelper::getJavaVM();
    JNIEnv *env             =   JniHelper::getEnv();
    vm->AttachCurrentThread(&env, nullptr);
    short buffer[_nInputSamples];
    _exitAudioRecorder      =   0;
    _skipFrameNum           =   0;
    while (!_exitAudioRecorder) {
        android_AudioIn(buffer, _nInputSamples);
    }
    vm->DetachCurrentThread();
}
void AudioOpenSL::stop(){
    if(_exitAudioRecorder == 0){
        _exitAudioRecorder       =   1;
        _conditionVar            =   0;
        _audioRecorderThread.join();
        _exitAudioRecorder       =   1;
        _conditionVar            =   0;
        android_CloseAudioDevice();
        _preDurationTime         =   0;
        _curDurationTime         =   0;
        if(_hEncoder != nullptr){
            faacEncClose(_hEncoder);
            _hEncoder    =   nullptr;
        }
        clearAudio();
    }
}

bool AudioOpenSL::android_OpenAudioDevice(){
    // create engine
    if(slCreateEngine(&_engineObject, 0, nullptr, 0, nullptr, nullptr) != SL_RESULT_SUCCESS){
        LOGE("slCreateEngine");
        return false;
    }
    // realize the engine
    if((*_engineObject)->Realize(_engineObject, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS){
        LOGE("Realize");
        return false;
    }
    // get the engine interface, which is needed in order to create other objects
    if((*_engineObject)->GetInterface(_engineObject, SL_IID_ENGINE, &_engineEngine) != SL_RESULT_SUCCESS){
        LOGE("GetInterface");
        return false;
    }
    // configure audio source
    SLDataLocator_IODevice loc_dev;
    loc_dev.locatorType             =   SL_DATALOCATOR_IODEVICE;
    loc_dev.deviceType              =   SL_IODEVICE_AUDIOINPUT;
    loc_dev.deviceID                =   SL_DEFAULTDEVICEID_AUDIOINPUT;
    loc_dev.device                  =   nullptr;
    SLDataSource audioSrc;
    audioSrc.pLocator               =   &loc_dev;
    audioSrc.pFormat                =   nullptr;

    SLDataLocator_AndroidSimpleBufferQueue loc_bq;
    loc_bq.locatorType              =   SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    loc_bq.numBuffers               =   1;
    SLDataFormat_PCM format_pcm;
    format_pcm.formatType           =   SL_DATAFORMAT_PCM;
    format_pcm.numChannels          =   CHANNELS;
    format_pcm.samplesPerSec        =   SL_SAMPLINGRATE_44_1;
    format_pcm.bitsPerSample        =   SL_PCMSAMPLEFORMAT_FIXED_16;
    format_pcm.containerSize        =   SL_PCMSAMPLEFORMAT_FIXED_16;
    // configure audio sink
    if(CHANNELS > 1){
        format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    }else {
        format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
    }
    format_pcm.endianness           =   SL_BYTEORDER_LITTLEENDIAN;
    SLDataSink audioSnk;
    audioSnk.pLocator               =   &loc_bq;
    audioSnk.pFormat                =   &format_pcm;
    // create audio recorder
    // (requires the RECORD_AUDIO permission)
    const SLInterfaceID id[1]      =   {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1]         =   {SL_BOOLEAN_TRUE};
    if (SL_RESULT_SUCCESS != (*_engineEngine)->CreateAudioRecorder(_engineEngine, &_recorderObject, &audioSrc,&audioSnk, 1, id, req)){
        LOGE("CreateAudioRecorder");
        return false;
    }
    // realize the audio recorder
    if (SL_RESULT_SUCCESS != (*_recorderObject)->Realize(_recorderObject, SL_BOOLEAN_FALSE)){
        LOGE("Realize");
        return false;
    }
    // get the record interface
    if (SL_RESULT_SUCCESS != (*_recorderObject)->GetInterface(_recorderObject, SL_IID_RECORD, &_recorderRecord)){
        LOGE("GetInterface");
        return false;
    }
    // get the buffer queue interface
    if (SL_RESULT_SUCCESS != (*_recorderObject)->GetInterface(_recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &_recorderBufferQueue)){
        LOGE("GetInterface");
        return false;
    }
    // register callback on the buffer queue
    if (SL_RESULT_SUCCESS != (*_recorderBufferQueue)->RegisterCallback(_recorderBufferQueue, AudioOpenSL::bqRecorderCallback, this)){
        LOGE("RegisterCallback");
        return false;
    }
    if (SL_RESULT_SUCCESS != (*_recorderRecord)->SetRecordState(_recorderRecord, SL_RECORDSTATE_RECORDING)){
        LOGE("SetRecordState");
        return false;
    }
    return true;
}

void AudioOpenSL::android_CloseAudioDevice(){
    // destroy audio recorder object, and invalidate all associated interfaces
    if (_recorderObject != nullptr) {
        (*_recorderObject)->Destroy(_recorderObject);
        _recorderObject      = nullptr;
        _recorderRecord      = nullptr;
        _recorderBufferQueue  = nullptr;
    }
    // destroy engine object, and invalidate all associated interfaces
    if (_engineObject != nullptr) {
        (*_engineObject)->Destroy(_engineObject);
        _engineObject        = nullptr;
        _engineEngine        = nullptr;
    }
}
// gets a buffer of size samples from the device
int AudioOpenSL::android_AudioIn(short *buffer,int size){
    auto index      = _currentInputIndex;
    auto inBuffer   = _audioInputBuffer[_currentInputBuffer];
    auto i          = 0;
    for(i=0; i < size; i++){
        if (index >= BUFFER_SIZE) {
            std::unique_lock<std::mutex> uLock(_mutex);
            _condVar.wait(uLock, [this]{ return _conditionVar == 0;});
            struct timeval curTime;
            gettimeofday(&curTime, 0);
            _curDurationTime =   curTime.tv_sec*1000    +   curTime.tv_usec/1000;
            _conditionVar    =   1;
            (*_recorderBufferQueue)->Enqueue(_recorderBufferQueue,inBuffer, BUFFER_SIZE*sizeof(short));
            _currentInputBuffer = (_currentInputBuffer ? 0 : 1);
            index            =   0;
            inBuffer         =   _audioInputBuffer[_currentInputBuffer];
        }
        buffer[i]            =   (short)inBuffer[index++];
    }
    _currentInputIndex = index;
    if (i > 0) {
        _skipFrameNum++;
        if(_skipFrameNum>=SKIP_FRAME_NUM){
            auto aacAudio   =   getWrite();
            if(aacAudio != nullptr && _curDurationTime > 0){
                aacAudio->state    =   AACAudio::AACAudio_STATE::WRITING;
                aacAudio->curTime  =   _curDurationTime;
                auto nRet          =   faacEncEncode(_hEncoder, (int *)buffer, i, (unsigned char*)aacAudio->data, _nMaxOutputBytes);
                aacAudio->size     =   nRet;
                aacAudio->state    =   AACAudio::AACAudio_STATE::READABLE;
            }
        }
    }
    return i;
}