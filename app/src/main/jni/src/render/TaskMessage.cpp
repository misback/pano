//
// Created by DELL on 2017/3/8.
//
#include "TaskMessage.h"
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
TaskMessage::TaskMessage():
_headLooperMessage(nullptr)
{
    sem_init(&_headDataAvailable, 0, 0);
    sem_init(&_headWriteProtect, 0, 1);
    _taskThread          =   std::thread(&TaskMessage::_taskLoop, this);
}
TaskMessage::~TaskMessage() {
    quit();
}
void TaskMessage::post(TaskMessageType what, WorkerData *_workerData, bool flush) {
    LooperMessage *msg = new LooperMessage();
    msg->what = what;
    msg->obj = _workerData;
    msg->next = nullptr;
    msg->quit = false;
    addMsg(msg, flush);
}
void TaskMessage::addMsg(LooperMessage *msg, bool flush) {
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
void TaskMessage::quit() {
    LooperMessage *msg = new LooperMessage();
    msg->what = TaskMessageType::kMsgQuit;
    msg->obj = nullptr;
    msg->next = nullptr;
    msg->quit = true;
    addMsg(msg, true);
    sem_post(&_headDataAvailable);
    _taskThread.join();
    sem_post(&_headDataAvailable);
    sem_destroy(&_headDataAvailable);
}

void TaskMessage::_taskLoop() {
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