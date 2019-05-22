//
// Created by DELL on 2017/3/8.
//
#include "AudioMediaCodec.h"
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
// aux effect on the output mix, used by the buffer queue player
static const SLEnvironmentalReverbSettings reverbSettings =SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
static void sbqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
AudioMediaCodec::AudioMediaCodec(AudioInterface* audioInterface) {
    _audioInterface = audioInterface;
}
AudioMediaCodec::~AudioMediaCodec() {
    if (running) {
        LOGE("Looper deleted while still running. Some messages will not be processed");
        quit();
    }
}

void AudioMediaCodec::quit() {
    LooperMessage *msg = new LooperMessage();
    msg->what = kMsgDecodeDone;
    msg->obj = &_workerData;
    msg->next = nullptr;
    msg->quit = true;
    addMsg(msg, true);
    sem_post(&_headDataAvailable);
    _mediaCodecThread.join();
    sem_post(&_headDataAvailable);
    sem_destroy(&_headDataAvailable);
    running = false;
    if(_workerData._aMediaCodec!=nullptr){
        AMediaCodec_stop(_workerData._aMediaCodec);
        AMediaCodec_delete(_workerData._aMediaCodec);
        _workerData._aMediaCodec = nullptr;
    }
    if(_workerData._aMediaExtractor!=nullptr){
        AMediaExtractor_delete(_workerData._aMediaExtractor);
        _workerData._aMediaExtractor = nullptr;
    }
    destroyOpenSL();
    _workerData._sawInputEOS = true;
    _workerData._sawOutputEOS = true;
}
void AudioMediaCodec::doCodecWork(WorkerData *d) {
    if(!d->_sawInputEOS && d->_aMediaCodec != nullptr){
        auto bufferId = AMediaCodec_dequeueInputBuffer(d->_aMediaCodec, 0);
        if (bufferId >= 0) {
            size_t bufferSize;
            auto buf = AMediaCodec_getInputBuffer(d->_aMediaCodec, bufferId, &bufferSize);
            auto sampleSize = AMediaExtractor_readSampleData(d->_aMediaExtractor, buf, bufferSize);
            auto presentationTimeUs = AMediaExtractor_getSampleTime(d->_aMediaExtractor);
            if (sampleSize < 0) {
                d->_sawInputEOS = true;
                AMediaCodec_queueInputBuffer(d->_aMediaCodec, bufferId, 0, 0, presentationTimeUs, AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
            }else{
                AMediaCodec_queueInputBuffer(d->_aMediaCodec, bufferId, 0, sampleSize, presentationTimeUs, 0);
                AMediaExtractor_advance(d->_aMediaExtractor);
            }
        }
    }
    if(!d->_sawOutputEOS && _workerData._aMediaCodec!=nullptr){
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
            if(info.size>0 && (*openSLStream.bqPlayerBufferQueue)!=nullptr){
                size_t out_size = 0;
                auto audioBuffer = AMediaCodec_getOutputBuffer(d->_aMediaCodec, status, &out_size);
                if(_signedStart == 1){
                    sem_wait(&_pcmDataAvailable);
                }else{
                    _signedStart = 1;
                }
                if(_audioInterface!=nullptr){
                    _audioInterface->callback(audioBuffer+info.offset, info.size);
                }
                if((*openSLStream.bqPlayerBufferQueue)!=nullptr){
                    (*openSLStream.bqPlayerBufferQueue)->Enqueue(openSLStream.bqPlayerBufferQueue, audioBuffer+info.offset, info.size);
                }
            }
            if(_workerData._aMediaCodec!=nullptr){
                AMediaCodec_releaseOutputBuffer(d->_aMediaCodec, status, info.size != 0);
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
void AudioMediaCodec::loadSource(AMediaExtractor* aMediaExtractor){
    WorkerData* workerData = new WorkerData();
    *workerData = _workerData;
    workerData->_aMediaExtractor = aMediaExtractor;
    post(kMsgLoadAudio, workerData);
}
void AudioMediaCodec::handle(LooperMessage* looperMessage) {
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
            destroyOpenSL();
            d->_sawInputEOS = true;
            d->_sawOutputEOS = true;
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
        case kMsgSeek: {
            WorkerData *d = (WorkerData*)obj;
            if(d->_aMediaExtractor!=nullptr && d->_aMediaCodec!=nullptr){
                AMediaExtractor_seekTo(d->_aMediaExtractor, d->_progress*d->_audioDuration, AMEDIAEXTRACTOR_SEEK_NEXT_SYNC);
                AMediaCodec_flush(d->_aMediaCodec);
                d->_start = -1;
                d->_sawInputEOS = false;
                d->_sawOutputEOS = false;
                if (!d->_isPlaying) {
                    d->_playOnce = true;
                }
            }
        }
        break;
        case kMsgPause: {
            WorkerData *d = (WorkerData*)obj;
            if (d->_isPlaying) {
                // flush all outstanding codecbuffer messages with a no-op message
                d->_isPlaying = false;
                sem_post(&_headDataAvailable);
                post(kMsgPauseAck, nullptr, true);
            }
        }
        break;
        case kMsgResume: {
            WorkerData *d = (WorkerData*)obj;
            if(d->_aMediaExtractor==nullptr || d->_aMediaCodec == nullptr){
                post(kMsgDecodeDone, d);
                return;
            }

            if(d->_sawInputEOS && d->_sawOutputEOS){
                AMediaExtractor_seekTo(d->_aMediaExtractor, 0, AMEDIAEXTRACTOR_SEEK_PREVIOUS_SYNC);
                AMediaCodec_flush(d->_aMediaCodec);
                d->_start = -1;
                d->_sawInputEOS = false;
                d->_sawOutputEOS = false;
                sem_post(&_headDataAvailable);
                post(kMsgCodecBuffer, d);
            }else if (!d->_isPlaying) {
                d->_start = -1;
                d->_isPlaying = true;
                sem_post(&_headDataAvailable);
                post(kMsgCodecBuffer, d);
            }
        }
        break;
        case kMsgLoadAudio:{
            if(_workerData._aMediaCodec != nullptr){
                AMediaCodec_stop(_workerData._aMediaCodec);
                AMediaCodec_delete(_workerData._aMediaCodec);
                _workerData._aMediaCodec = nullptr;
            }
            if(_workerData._aMediaExtractor != nullptr){
                AMediaExtractor_delete(_workerData._aMediaExtractor);
                _workerData._aMediaExtractor = nullptr;
            }
            destroyOpenSL();
            WorkerData* newWorkerData = (WorkerData*)obj;
            _workerData._aMediaExtractor = newWorkerData->_aMediaExtractor;
            createOpenSL(_workerData._sampleRate, _workerData._channels);
            delete ((WorkerData*)looperMessage->obj);
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
                if (!strncmp(mime, "audio/", 6)) {
                    LOGE("======%s", mime);
                    AMediaExtractor_selectTrack(_workerData._aMediaExtractor, i);
                    auto codecAudio = AMediaCodec_createDecoderByType(mime);
                    int32_t sampleRate = 0;
                    int32_t channels = 0;
                    AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, &sampleRate);
                    AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &channels);
                    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 0);
                    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_BIT_RATE, 48000);
                    AMediaFormat_getInt64(format, AMEDIAFORMAT_KEY_DURATION, &(_workerData._audioDuration));
                    AMediaCodec_configure(codecAudio, format, nullptr, nullptr, 0);
                    AMediaFormat_delete(format);
                    _workerData._aMediaCodec = codecAudio;
                    _workerData._sampleRate = sampleRate;
                    _workerData._channels = channels;
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
// this callback handler is called every time a buffer finishes playing
void sbqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context){
    auto audioMediaCodec = (AudioMediaCodec *) context;
    audioMediaCodec->bqPlayerCallback();
}
void AudioMediaCodec::bqPlayerCallback(){
    sem_post(&_pcmDataAvailable);
}
void AudioMediaCodec::createOpenSL(int sampleRate, int channels){
    if( // create engine
        SL_RESULT_SUCCESS != slCreateEngine(&(openSLStream.engineObject), 0, NULL, 0, NULL, NULL)
        // realize the engine
        || SL_RESULT_SUCCESS != (*openSLStream.engineObject)->Realize(openSLStream.engineObject, SL_BOOLEAN_FALSE)
        // get the engine interface, which is needed in order to create other objects
        || SL_RESULT_SUCCESS != (*openSLStream.engineObject)->GetInterface(openSLStream.engineObject, SL_IID_ENGINE, &(openSLStream.engineEngine))
        ){
            destroyOpenSL();
            LOGE("error in engine");
            return;
        }
    const SLInterfaceID ids[] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[] = {SL_BOOLEAN_FALSE};
    if( // create output mix, with environmental reverb specified as a non-required interface
        SL_RESULT_SUCCESS != (*openSLStream.engineEngine)->CreateOutputMix(openSLStream.engineEngine, &(openSLStream.outputMixObject), 1, ids, req)
        // realize the output mix
        || SL_RESULT_SUCCESS != (*openSLStream.outputMixObject)->Realize(openSLStream.outputMixObject, SL_BOOLEAN_FALSE)
        // get the environmental reverb interface
        // this could fail if the environmental reverb effect is not available,
        // either because the feature is not present, excessive CPU load, or
        // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
        || SL_RESULT_SUCCESS != (*openSLStream.outputMixObject)->GetInterface(openSLStream.outputMixObject, SL_IID_ENVIRONMENTALREVERB, &(openSLStream.outputMixEnvironmentalReverb))
        || SL_RESULT_SUCCESS != (*openSLStream.outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties( openSLStream.outputMixEnvironmentalReverb, &reverbSettings)
        ){
            destroyOpenSL();
            LOGE("error in mix");
            return;
        }
    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLuint32 sr = sampleRate;
    switch(sr){
        case 8000:
            sr = SL_SAMPLINGRATE_8;
            break;
        case 11025:
            sr = SL_SAMPLINGRATE_11_025;
            break;
        case 16000:
            sr = SL_SAMPLINGRATE_16;
            break;
        case 22050:
            sr = SL_SAMPLINGRATE_22_05;
            break;
        case 24000:
            sr = SL_SAMPLINGRATE_24;
            break;
        case 32000:
            sr = SL_SAMPLINGRATE_32;
            break;
        case 44100:
            sr = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            sr = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            sr = SL_SAMPLINGRATE_64;
            break;
        case 88200:
            sr = SL_SAMPLINGRATE_88_2;
            break;
        case 96000:
            sr = SL_SAMPLINGRATE_96;
            break;
        case 192000:
            sr = SL_SAMPLINGRATE_192;
            break;
        default:
            LOGE("error:sample rate");
            break;
    }
    SLuint32 speakers = (channels > 1?(SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT):SL_SPEAKER_FRONT_CENTER);
    SLDataFormat_PCM format_pcm = {
        SL_DATAFORMAT_PCM,
        (SLuint32)channels,
        (SLuint32)sr,
        SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_PCMSAMPLEFORMAT_FIXED_16,
        speakers,
        SL_BYTEORDER_LITTLEENDIAN
        };
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};
    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, openSLStream.outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};
    // create audio player
    const SLInterfaceID ids1[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,/*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
    const SLboolean req1[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,/*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};
    if(
        SL_RESULT_SUCCESS != (*openSLStream.engineEngine)->CreateAudioPlayer(openSLStream.engineEngine, &(openSLStream.bqPlayerObject), &audioSrc, &audioSnk, 3, ids1, req1)
        // realize the player
        || SL_RESULT_SUCCESS != (*openSLStream.bqPlayerObject)->Realize(openSLStream.bqPlayerObject, SL_BOOLEAN_FALSE)
        // get the play interface
        || SL_RESULT_SUCCESS != (*openSLStream.bqPlayerObject)->GetInterface(openSLStream.bqPlayerObject, SL_IID_PLAY, &(openSLStream.bqPlayerPlay))
        // get the buffer queue interface
        || SL_RESULT_SUCCESS != (*openSLStream.bqPlayerObject)->GetInterface(openSLStream.bqPlayerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &(openSLStream.bqPlayerBufferQueue))
        // register callback on the buffer queue
        || SL_RESULT_SUCCESS != (*openSLStream.bqPlayerBufferQueue)->RegisterCallback(openSLStream.bqPlayerBufferQueue, sbqPlayerCallback, this)
        // get the effect send interface
        || SL_RESULT_SUCCESS != (*openSLStream.bqPlayerObject)->GetInterface(openSLStream.bqPlayerObject, SL_IID_EFFECTSEND, &openSLStream.bqPlayerEffectSend)
        // get the volume interface
        || SL_RESULT_SUCCESS != (*openSLStream.bqPlayerObject)->GetInterface(openSLStream.bqPlayerObject, SL_IID_VOLUME, &openSLStream.bqPlayerVolume)
        // set the player's state to playing
        || SL_RESULT_SUCCESS != (*openSLStream.bqPlayerPlay)->SetPlayState(openSLStream.bqPlayerPlay, SL_PLAYSTATE_PLAYING)
        ){
            destroyOpenSL();
            LOGE("error in player");
            return;
        }
    sem_init(&_pcmDataAvailable, 0, 0);
    _signedStart = 0;
}
// close the OpenSL IO and destroy the audio engine
void AudioMediaCodec::destroyOpenSL(){
    sem_post(&_headDataAvailable);
    sem_destroy(&_headDataAvailable);
    _signedStart = 0;
    // destroy buffer queue audio player object, and invalidate all associated interfaces
    if (openSLStream.bqPlayerObject != NULL) {
        (*openSLStream.bqPlayerObject)->Destroy(openSLStream.bqPlayerObject);
        openSLStream.bqPlayerObject = NULL;
        openSLStream.bqPlayerPlay = NULL;
        openSLStream.bqPlayerBufferQueue = NULL;
        openSLStream.bqPlayerEffectSend = NULL;
        openSLStream.bqPlayerMuteSolo = NULL;
        openSLStream.bqPlayerVolume = NULL;
    }

    // destroy output mix object, and invalidate all associated interfaces
    if (openSLStream.outputMixObject != NULL) {
        (*openSLStream.outputMixObject)->Destroy(openSLStream.outputMixObject);
        openSLStream.outputMixObject = NULL;
        openSLStream.outputMixEnvironmentalReverb = NULL;
    }

    // destroy engine object, and invalidate all associated interfaces
    if (openSLStream.engineObject != NULL) {
        (*openSLStream.engineObject)->Destroy(openSLStream.engineObject);
        openSLStream.engineObject = NULL;
        openSLStream.engineEngine = NULL;
    }
}