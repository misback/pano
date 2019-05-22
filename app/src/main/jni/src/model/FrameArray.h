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

//注意:不可能解码速度达不到USB的1/60
struct FrameElement{
    enum class FRAME_STATE{
        IDLEING     =   0,
        READING     =   1,
        WRITING     =   2,
        READABLE    =   3
    };
    unsigned char*             mUvcFrameData;
    size_t                       mSize;
    //volatile FRAME_STATE        mState;
    std::atomic<FRAME_STATE>         mState;
    bool                        mIsKeyFrame;
    FrameElement(){
        mIsKeyFrame     =   false;
        mUvcFrameData   =   nullptr;
        mState          =   FRAME_STATE::IDLEING;
        mSize           =   0;
    }
    ~FrameElement(){
        if(mUvcFrameData != nullptr){
             delete[] mUvcFrameData;
            mUvcFrameData   =   nullptr;
        }
        mIsKeyFrame     =   false;
        mState          =   FRAME_STATE::IDLEING;
        mSize           =   0;
    }
};
class FrameArray {
private:
    const static int MAX_SIZE   =   60;
    const static int FRAME_RATE =   30;
    std::vector<FrameElement*> mDataVec;
    unsigned int mSize;
    unsigned int mReadPos;
    unsigned int mWritePos;
public:
	FrameArray() {
        mSize       =   MAX_SIZE;
        mReadPos    =   0;
        mWritePos   =   0;
        mDataVec.reserve(MAX_SIZE);
        mDataVec    =   std::vector<FrameElement*>(MAX_SIZE, nullptr);
	}
	~FrameArray() {
	    for(auto element:mDataVec){
	        if(element != nullptr){
	            delete element;
	        }
	    }
	    mDataVec.clear();
	}

	void clear(){
	    auto size   =   mDataVec.size();
	    for(auto i=0; i<size; i++){
	        if(mDataVec[i] != nullptr){
	            delete mDataVec[i], mDataVec[i] =   nullptr;
	        }
	    }
	}

	void write(FrameElement* fElement){
        if(mDataVec[mWritePos] != nullptr && mDataVec[mWritePos]->mState == FrameElement::FRAME_STATE::READING){
            mWritePos   =  (mWritePos+1)%MAX_SIZE;
        }
        SAFE_SET(mDataVec[mWritePos], fElement)
        mWritePos       =  (mWritePos+1)%MAX_SIZE;
	}

	int getReadElementNum(){
	    int num =   0;
	    for(auto element:mDataVec){
	        if(element != nullptr && element->mState == FrameElement::FRAME_STATE::READABLE){
                num ++;
	        }
	    }
	    return num;
	}

	FrameElement* read(int frameRate){
	    int readNum                 =   getReadElementNum();
	    FrameElement* preElement    =   nullptr;
	    auto skipNum                =   0;
	    if(readNum > 0){
	        for(int i=mReadPos; i<(mReadPos+MAX_SIZE); i++){
                int pos =   i%MAX_SIZE;
                FrameElement* readElement   =   mDataVec[pos];
                if(readElement != nullptr && readElement->mState  ==  FrameElement::FRAME_STATE::READABLE){
                    if(preElement != nullptr){
                        preElement->mState     =   FrameElement::FRAME_STATE::IDLEING;
                    }
                    skipNum++;
                    mReadPos = (pos+1) % MAX_SIZE;
                    readElement->mState     =   FrameElement::FRAME_STATE::READING;
                    preElement  =   readElement;
                    if((readNum-skipNum)<3){
                        return preElement;
                    }
                    if(preElement->mIsKeyFrame){
                        return preElement;
                    }
                }
	        }
	        return preElement;
	    }
        return nullptr;
	}
};
