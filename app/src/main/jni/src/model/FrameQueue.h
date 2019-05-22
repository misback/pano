#pragma once
#include "Common.h"
#include <vector>
#include <atomic>
#define SAFE_SET(A,B) \
    if((A) != nullptr){\
        delete (A);\
        (A) =   nullptr;\
    }\
    (A) = (B);\
struct Frame{
    unsigned char*             data;
    size_t                       size;
    bool                        iskey;
    Frame(){
        iskey  =   false;
        data   =   nullptr;
        size   =   0;
    }
    ~Frame(){
        if(data != nullptr){
            delete[] data;
            data = nullptr;
        }
        iskey = false;
        size  = 0;
    }
};