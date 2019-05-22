//
// Created by DELL on 2017/3/8.
//
#include "TaskFaceMessage.h"
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
TaskFaceMessage::TaskFaceMessage():
_headFaceMessage(nullptr),
_startFace(0)
{
    sem_init(&_headFaceDataAvailable, 0, 0);
    sem_init(&_headFaceWriteProtect, 0, 1);
    _taskFaceThread          =   std::thread(&TaskFaceMessage::_taskFaceLoop, this);
}
TaskFaceMessage::~TaskFaceMessage() {
    quitFace();
}
void TaskFaceMessage::postFace(FaceMessageType what, FaceData *faceData, bool flush) {
    FaceMessage *msg = new FaceMessage();
    msg->what = what;
    msg->obj = faceData;
    msg->next = nullptr;
    msg->quit = false;
    addFaceMsg(msg, flush);
}
void TaskFaceMessage::addFaceMsg(FaceMessage *msg, bool flush) {
    sem_wait(&_headFaceWriteProtect);
    FaceMessage *h = _headFaceMessage;
    if (flush) {
        while(h) {
            FaceMessage *next = h->next;
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
        _headFaceMessage = msg;
    }
    sem_post(&_headFaceWriteProtect);
    sem_post(&_headFaceDataAvailable);
}
void TaskFaceMessage::quitFace() {
    FaceMessage *msg = new FaceMessage();
    msg->what = FaceMessageType::kMsgQuit;
    msg->obj = nullptr;
    msg->next = nullptr;
    msg->quit = true;
    addFaceMsg(msg, true);
    sem_post(&_headFaceDataAvailable);
    _taskFaceThread.join();
    sem_post(&_headFaceDataAvailable);
    sem_destroy(&_headFaceDataAvailable);
}

void TaskFaceMessage::_taskFaceLoop() {
    while(true) {
        // wait for available message
        sem_wait(&_headFaceDataAvailable);
        // get next available message
        sem_wait(&_headFaceWriteProtect);
        FaceMessage *msg = _headFaceMessage;
        if (msg == nullptr) {
            sem_post(&_headFaceWriteProtect);
            continue;
        }
        _headFaceMessage = msg->next;
        sem_post(&_headFaceWriteProtect);
        if (msg->quit) {
            LOGE("quitting");
            delete msg;
            return;
        }
        handleFace(msg);
        delete msg;
    }
}