#include "MP4Encoder.h"
#include <string.h>
#include "JniHelper.h"
#include "Common.h"
#include "AudioOpenSL.h"
#define VIDEO_BUF_SIZE          (1024*500)
#define PPS_SIGN                0x68
#define SPS_SIGN                0x67
#define DEFAULT_SKIP_FRAME_NUM  15
static void make_dsi(unsigned int sampling_frequency_index, unsigned int channel_configuration, unsigned char* dsi){
    unsigned int object_type = 2;
    dsi[0] = (object_type<<3)|(sampling_frequency_index>>1);
    dsi[1] = ((sampling_frequency_index&1)<<7) | (channel_configuration<<3);
}
static int get_sr_index(unsigned int sampling_frequency){
    switch(sampling_frequency){
        case 96000:     return 0;
        case 88200:     return 1;
        case 64000:     return 2;
        case 48000:     return 3;
        case 44100:     return 4;
        case 32000:     return 5;
        case 24000:     return 6;
        case 22050:     return 7;
        case 16000:     return 8;
        case 12000:     return 9;
        case 11025:     return 10;
        case 8000:      return 11;
        case 7350:      return 12;
        default:        return 0;
    }
}
template<> MP4Encoder* Singleton<MP4Encoder>::msSingleton = nullptr;
MP4Encoder::MP4Encoder(void):  
            _videoTrackId(0),
            _nWidth(0),
            _nHeight(0),
            _nTimeScale(DEFAULT_MP4_TIME_SCALE),
            _nFrameRate(DEFAULT_MP4_FRAME_RATE),
            _nPreTime(0),
            _recordState(RECORD::END),
            _hMP4FileHandle(nullptr),
            _videoBuf(nullptr),
            _skipFrameNum(0){
    _videoBuf   =   new unsigned char[VIDEO_BUF_SIZE];
    if(AudioOpenSL::getSingletonPtr() == nullptr){
        new AudioOpenSL();
    }
}
MP4Encoder::~MP4Encoder(void)  {
    if(_videoBuf != nullptr){
        delete[] _videoBuf;
        _videoBuf   =   nullptr;
    }
    if(AudioOpenSL::getSingletonPtr() != nullptr){
        delete AudioOpenSL::getSingletonPtr();
    }
}
void MP4Encoder::init(const char* _mediaDir, int frameRate, int timeScale, int width, int height){
    _sMediaDir             =   _mediaDir;
    _nFrameRate            =   frameRate;
    _nTimeScale            =   timeScale;
    _nWidth                =   width;
    _nHeight               =   height;
    _hMP4FileHandle        =   nullptr;
}

int MP4Encoder::startRecording(){
    auto result =   0;
    switch(_recordState){
        case RECORD::START:
        case RECORD::RUNNING:
            break;
        case RECORD::END:
        {
            if(AudioOpenSL::getSingletonPtr()->startRecording() == 1){
                _recordState   =  RECORD::START;
                result          = 1;
                GEN_CAMERA_MP4_FILENAME(_sFileName, _sMediaDir.c_str(), "mp4");
            }
        }
            break;
    }
    return result;
}

std::string MP4Encoder::stopRecording(){
    _recordState   =  RECORD::END;
    AudioOpenSL::getSingletonPtr()->stop();
    if(_hMP4FileHandle != nullptr){
        closeMP4File();
        _hMP4FileHandle  =   nullptr;
        return _sFileName;
    }
    return "";
}
void MP4Encoder::recordingData(const unsigned char* pData, size_t size, bool isKeyFrame){
    switch(_recordState){
        case RECORD::START:{
            if(isKeyFrame){
                if(createMP4File(_sFileName.c_str())){
                    _recordState      =   RECORD::RUNNING;
                    writeH264Data(pData, size, isKeyFrame);
                }
            }
        }
            break;
        case RECORD::RUNNING:{
            writeH264Data(pData, size, isKeyFrame);
        }
            break;
        case RECORD::END:{

        }
            break;
    }
}
bool MP4Encoder::createMP4File(const char *pFileName){
	_hMP4FileHandle = MP4CreateEx(pFileName, 0, 1, 1, 0, 0, 0, 0);//创建mp4文件
	if (_hMP4FileHandle != MP4_INVALID_FILE_HANDLE){
        MP4SetTimeScale(_hMP4FileHandle, _nTimeScale);
        _videoTrackId = MP4AddH264VideoTrack(_hMP4FileHandle, _nTimeScale, _nTimeScale / _nFrameRate, _nWidth, _nHeight,
                                                _dMP4ENC_Metadata.Sps[1], // sps[1] AVCProfileIndication
                                                _dMP4ENC_Metadata.Sps[2], // sps[2] profile_compat
                                                _dMP4ENC_Metadata.Sps[3], // sps[3] AVCLevelIndication
                                                3);                       // 4 bytes length before each NAL unit
        //添加aac音频
        _audioTrackId = MP4AddAudioTrack(_hMP4FileHandle, AUDIO_SAMPLE_RATE, AUDIO_DURATION, MP4_MPEG4_AUDIO_TYPE);
        if (_audioTrackId == MP4_INVALID_TRACK_ID){
            MP4Close(_hMP4FileHandle, 0);
            return false;
        }
        unsigned char p_config[2] = {0x15, 0x88};
        make_dsi(get_sr_index(AUDIO_SAMPLE_RATE), CHANNELS, p_config);
        MP4SetTrackESConfiguration(_hMP4FileHandle, _audioTrackId, p_config, 2);
        MP4SetAudioProfileLevel(_hMP4FileHandle, 0x02);
        if (_videoTrackId != MP4_INVALID_TRACK_ID){
            MP4AddH264SequenceParameterSet(_hMP4FileHandle,_videoTrackId,_dMP4ENC_Metadata.Sps,_dMP4ENC_Metadata.nSpsLen);
            MP4AddH264PictureParameterSet(_hMP4FileHandle,_videoTrackId,_dMP4ENC_Metadata.Pps,_dMP4ENC_Metadata.nPpsLen);
            MP4SetVideoProfileLevel(_hMP4FileHandle, 0x7F);
            return true;
        }else{
            MP4Close(_hMP4FileHandle, 0);
            _hMP4FileHandle  =   nullptr;
        }
	}
	return false;
}
void MP4Encoder::closeMP4File(){
    if(_hMP4FileHandle){
        MP4Close(_hMP4FileHandle);
        _hMP4FileHandle = nullptr;
    }
}
void MP4Encoder::writeH264Data(const unsigned char* pData,size_t size, bool isKeyFrame){
    memcpy(_videoBuf, pData, size);
    if(isKeyFrame){
        auto nal_offset     = _dMP4ENC_Metadata.nPpsOffset + 8;
        auto nal_size       = size - nal_offset;
        _videoBuf[0]        = 0;
        _videoBuf[1]        = 0;
        _videoBuf[2]        = 0;
        _videoBuf[3]        = 2;
        _videoBuf[_dMP4ENC_Metadata.nSpsOffset - 4] = 0;
        _videoBuf[_dMP4ENC_Metadata.nSpsOffset - 3] = 0;
        _videoBuf[_dMP4ENC_Metadata.nSpsOffset - 2] = 0;
        _videoBuf[_dMP4ENC_Metadata.nSpsOffset - 1] = _dMP4ENC_Metadata.nSpsLen;
        _videoBuf[_dMP4ENC_Metadata.nPpsOffset - 4] = 0;
        _videoBuf[_dMP4ENC_Metadata.nPpsOffset - 3] = 0;
        _videoBuf[_dMP4ENC_Metadata.nPpsOffset - 2] = 0;
        _videoBuf[_dMP4ENC_Metadata.nPpsOffset - 1] = _dMP4ENC_Metadata.nPpsLen;
        _videoBuf[nal_offset - 4] = (nal_size & 0xff000000) >> 24;
        _videoBuf[nal_offset - 3] = (nal_size & 0x00ff0000) >> 16;
        _videoBuf[nal_offset - 2] = (nal_size & 0x0000ff00) >> 8;
        _videoBuf[nal_offset - 1] = nal_size & 0x000000ff;
    }else{
        _videoBuf[0] = 0;
        _videoBuf[1] = 0;
        _videoBuf[2] = 0;
        _videoBuf[3] = 2;
        auto nal_size = size - 10;
        _videoBuf[6] = (nal_size & 0xff000000) >> 24;
        _videoBuf[7] = (nal_size & 0x00ff0000) >> 16;
        _videoBuf[8] = (nal_size & 0x0000ff00) >> 8;
        _videoBuf[9] = nal_size & 0x000000ff;
    }
    auto duration    =   MP4_INVALID_DURATION;
    auto timestamp   =   0;
    if(_nPreTime != 0){
        struct timeval curTime;
        gettimeofday(&curTime, 0);
        timestamp   =   curTime.tv_sec*1000    +   curTime.tv_usec/1000;
        duration    =   timestamp   -   _nPreTime;
    }
    _nPreTime  =   timestamp;
    MP4WriteSample(_hMP4FileHandle, _videoTrackId, _videoBuf, size, duration, 0, isKeyFrame?1:0);
    AudioOpenSL::getSingletonPtr()->writeAACAudioOpenSL(_hMP4FileHandle, _audioTrackId, duration, isKeyFrame?1:0);
}

bool MP4Encoder:: praseMetadata(const unsigned char* pData,size_t size){
    auto i = 0;
    while(i<size){
        if(pData[i++] == 0x00 && pData[i++] == 0x00 && pData[i++] == 0x00 && pData[i++] == 0x01){
            if(pData[i] == SPS_SIGN){
                _dMP4ENC_Metadata.nSpsOffset = i;
            }
            if(pData[i] == PPS_SIGN){
                _dMP4ENC_Metadata.nPpsOffset = i;
                _dMP4ENC_Metadata.nSpsLen    =   _dMP4ENC_Metadata.nPpsOffset - _dMP4ENC_Metadata.nSpsOffset - 4;
                memcpy(_dMP4ENC_Metadata.Sps, pData + _dMP4ENC_Metadata.nSpsOffset, _dMP4ENC_Metadata.nSpsLen);
                _dMP4ENC_Metadata.nPpsLen    =   4;
                memcpy(_dMP4ENC_Metadata.Pps, pData + _dMP4ENC_Metadata.nPpsOffset, 4);
                return true;
            }
        }
    }
    return false;
}
