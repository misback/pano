//
// Created by DELL on 2017/3/8.
//
#pragma once
#include <semaphore.h>
#include <thread>
#include <atomic>
#include <string.h>
#include "Common.h"
class TaskMessage{
public:
    enum class TaskMessageType{
        kMsgSaveData,
        kMsgFilterSaveData,
        kMsgQuit,
    };
    struct WorkerData{
        WorkerData& operator=(const WorkerData& ) = delete;
        WorkerData(WorkerData&) = delete;
        unsigned char* _data;
        int _width;
        int _height;
        ssize_t _size;
        std::string _dir;
        WorkerData(){
            _data = nullptr;
            _width = 0;;
            _height = 0;
            _size = 0;
        }
        WorkerData(unsigned char* data, int width, int height, ssize_t size){
            _data = new unsigned char[size];
            memcpy(_data, data, size);
            _width = width;
            _height = height;
            _size = size;
        }

        WorkerData(unsigned char* data, int width, int height, ssize_t size, const char* dir){
            _data = new unsigned char[size];
            memcpy(_data, data, size);
            _width = width;
            _height = height;
            _size = size;
            _dir = dir;
        }

        ~WorkerData(){
            if(_data!=nullptr){
                delete[] _data, _data = nullptr;
            }
            _width = 0;
            _height = 0;
            _size = 0;
        }
    };
    struct LooperMessage {
        LooperMessage& operator=(const LooperMessage& ) = delete;
        LooperMessage(LooperMessage&) = delete;
        TaskMessageType what;
        WorkerData *obj;
        LooperMessage *next;
        bool quit;
        LooperMessage(){
             obj  = nullptr;
             next = nullptr;
             quit = false;
        }
        ~LooperMessage(){
            if(obj != nullptr){
                delete obj;
            }
            obj  = nullptr;
        }
    };
    public:
        TaskMessage();
        TaskMessage& operator=(const TaskMessage& ) = delete;
        TaskMessage(TaskMessage&) = delete;
        virtual ~TaskMessage();
        virtual void handle(LooperMessage* looperMessage) = 0;
    protected:
        void post(TaskMessageType what, WorkerData *data, bool flush = false);
    private:
        void quit();
        void addMsg(LooperMessage *msg, bool flush);
        LooperMessage *_headLooperMessage;
        void _taskLoop();
        std::thread _taskThread;
        sem_t _headWriteProtect;
        sem_t _headDataAvailable;
};
