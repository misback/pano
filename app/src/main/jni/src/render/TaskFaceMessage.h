//
// Created by DELL on 2017/3/8.
//
#pragma once
#include <semaphore.h>
#include <thread>
#include <atomic>
#include "Common.h"
#include <string.h>
#define FACE_FRAME_RATIO 1
class TaskFaceMessage{
public:
    enum class FaceMessageType {
        kMsgStart,
        kMsgFaceDetectorData,
        kMsgEnd,
        kMsgQuit,
    };
    struct FaceData{
        FaceData& operator=(const FaceData& ) = delete;
        FaceData(FaceData&) = delete;
        unsigned char* _data;
        FaceData(){
            _data = nullptr;
        }
        FaceData(unsigned char* data, ssize_t size){
            _data = new unsigned char[size];
            memcpy(_data, data, size);
        }

        FaceData(unsigned char* data, ssize_t width, ssize_t height){
            _data = new unsigned char[width*height];
            for(auto h=0; h<height; h++){
                for(auto w=0; w<width; w++){
                    _data[h*width+w] = data[h*width*4+w*4];
                }
            }
        }

        ~FaceData(){
            if(_data!=nullptr){
                delete[] _data, _data = nullptr;
            }
        }
    };
    struct FaceMessage {
        FaceMessage& operator=(const FaceMessage& ) = delete;
        FaceMessage(FaceMessage&) = delete;
        FaceMessageType what;
        FaceData *obj;
        FaceMessage *next;
        bool quit;
        FaceMessage(){
             obj  = nullptr;
             next = nullptr;
             quit = false;
        }
        ~FaceMessage(){
            if(obj != nullptr){
                delete obj;
            }
            obj  = nullptr;
        }
    };
    public:
        TaskFaceMessage();
        TaskFaceMessage& operator=(const TaskFaceMessage& ) = delete;
        TaskFaceMessage(TaskFaceMessage&) = delete;
        virtual ~TaskFaceMessage();
        virtual void handleFace(FaceMessage* looperMessage) = 0;
    protected:
        std::atomic<int> _startFace;
        void postFace(FaceMessageType what, FaceData *data, bool flush = false);
    private:
        void quitFace();
        void addFaceMsg(FaceMessage *msg, bool flush);
        FaceMessage *_headFaceMessage;
        void _taskFaceLoop();
        std::thread _taskFaceThread;
        sem_t _headFaceWriteProtect;
        sem_t _headFaceDataAvailable;
};
