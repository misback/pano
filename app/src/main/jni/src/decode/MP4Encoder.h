#pragma once
#include "mp4v2/mp4v2.h"
#include "Singleton.h"
#include "string.h"
#include <stdio.h>
#include "Common.h"
#include <sstream>
#include <thread>
#define DEFAULT_MP4_FRAME_RATE      30
#define DEFAULT_MP4_TIME_SCALE      90000
// NALU单元  
typedef struct _MP4ENC_NaluUnit{
    int type;  
    int size;  
    unsigned char *data;  
}MP4ENC_NaluUnit;  
  
typedef struct _MP4ENC_Metadata{
    // video, must be h264 type
    unsigned int    nSpsOffset;
    unsigned int    nSpsLen;
    unsigned char   Sps[1024];
    unsigned int    nPpsOffset;
    unsigned int    nPpsLen;  
    unsigned char   Pps[1024];  
  
}MP4ENC_Metadata,*LPMP4ENC_Metadata;
  
class MP4Encoder : public Singleton<MP4Encoder>
{
    enum class RECORD{
        START       =   0,
        RUNNING     =   1,
        END         =   2
    };
public:  
    MP4Encoder(void);  
    ~MP4Encoder(void);
    void init(const char* _mediaDir, int frameRate, int timeScale, int width, int height);
    int startRecording();
    std::string stopRecording();
    bool createMP4File(const char *fileName);
    void writeH264Data(const unsigned char* pData,size_t size, bool isKeyFrame);
    void closeMP4File();
    bool praseMetadata(const unsigned char* pData,size_t size);
    void recordingData(const unsigned char* pData, size_t size, bool isKeyFrame);
    std::thread _audioThread;
private:
    std::string         _sMediaDir;
    std::string         _sThumbnailMediaDir;
    std::string         _sFileName;
    RECORD              _recordState;
    MP4ENC_Metadata     _dMP4ENC_Metadata;
    MP4TrackId          _videoTrackId;
    MP4FileHandle       _hMP4FileHandle;
    MP4TrackId          _audioTrackId;

    unsigned char*    _videoBuf;
    long               _nPreTime;
    int                _nWidth;
    int                _nHeight;
    int                _nFrameRate;
    int                _nTimeScale;
    int                _skipFrameNum;
};   