//
// Created by DELL on 2017/3/8.
//
#include "MediaCodec.h"
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
MediaCodec::MediaCodec():
_headLooperMessage(nullptr)
{
    sem_init(&_headDataAvailable, 0, 0);
    sem_init(&_headWriteProtect, 0, 1);
    running = true;
    _mediaCodecThread          =   std::thread(&MediaCodec::_mediaCodecLoop, this);
}
MediaCodec::~MediaCodec() {
}
void MediaCodec::post(int what, void *_workerData, bool flush) {
    LooperMessage *msg = new LooperMessage();
    msg->what = what;
    msg->obj = _workerData;
    msg->next = nullptr;
    msg->quit = false;
    addMsg(msg, flush);
}
void MediaCodec::addMsg(LooperMessage *msg, bool flush) {
    sem_wait(&_headWriteProtect);
    LooperMessage *h = _headLooperMessage;
    if (flush) {
        while(h) {
            LooperMessage *next = h->next;
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
        _headLooperMessage = msg;
    }
    sem_post(&_headWriteProtect);
    sem_post(&_headDataAvailable);
}
void MediaCodec::_mediaCodecLoop() {
    while(true) {
        // wait for available message
        sem_wait(&_headDataAvailable);
        // get next available message
        sem_wait(&_headWriteProtect);
        LooperMessage *msg = _headLooperMessage;
        if (msg == nullptr) {
            sem_post(&_headWriteProtect);
            continue;
        }
        _headLooperMessage = msg->next;
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

void MediaCodec::nativeSetPlaying(bool isPlaying){
    post(isPlaying?kMsgResume:kMsgPause, &_workerData, true);
}
void MediaCodec::nativeShutDown(){
    post(kMsgDecodeDone, &_workerData, true /* flush */);
    quit();
}
void MediaCodec::nativeUpdateProgress(float progress){
    _workerData._progress = progress;
    post(kMsgSeek, &_workerData, true);
}
void MediaCodec::nativeRestart(){
    post(kMsgReStart, &_workerData, true);
}