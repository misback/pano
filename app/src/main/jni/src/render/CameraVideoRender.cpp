#include "CameraVideoRender.h"
#include <stdio.h>
#include <jni.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include "math.h"
#include "ImageUtil.h"
#include "shader/panorama_position.vert"
#include "shader/panorama_play.frag"
#include "shader/panorama_face_position.vert"
#include "shader/panorama_face_play.frag"
#include "shader/filter_camera_position.vert"
#include "shader/filter_camera_play.frag"
#include "face/FaceDetectManager.h"
#include "Bitmap.h"
#define MIME_TYPE "video/avc"
static volatile int video_connected = 0;
using namespace std;
CameraVideoRender::CameraVideoRender(const char* tDetectModelPath, const char* tAlignModelPath):
    _widgetWidth(0),
    _widgetHeight(0),
    _recordWidth(0),
    _recordHeight(0),
    _takePhotoTickCount(0),
    _takePhotoWidth(0),
    _takePhotoHeight(0),
    _faceWidth(0),
    _faceHeight(0),
    _faceCalWidth(0),
    _faceCalHeight(0),
    _useFaceDetector(0),
    _sProgramPlay(0),
    _aVertexPosHandlePlay(0),
    _aVertexUVHandlePlay(0),
    _uMvpMatrixLocPlay(0),
    _uFilterLutSizeLocPlay(0),
    _lutSize(0),
    _uFilterLutTextureLocPlay(0),
    _lutTextureRGB(nullptr),
    _uLogoAngleLocPlay(0),
    _logoAngle(15.0),
    _uFilterLogoTextureLocPlay(0),
    _uFilterLogoAngleLocPlay(0),
    _uEffectLatitudeLocPlay(0),
    _uEffectLongitudeLocPlay(0),
    _uEffectFaceSizeLocPlay(0),
    _uFilterEffectLatitudeLocPlay(0),
    _uFilterEffectLongitudeLocPlay(0),
    _uFilterEffectFaceSizeLocPlay(0),
    _latitude(0.f),
    _longitude(0.f),
    _faceSize(0.f),
    _aAssetManager(nullptr),
    _renderCallbackObj(nullptr),
    _callbackTakePhotoMethod(nullptr),
    _scale(SCALE_DEFAULT_FISH),
    _xRotateAngle(0.0),
    _yRotateAngle(0.0),
    _fovAngle(FOV_DEFAULT_FISH),
    _zNear(Z_NEAR_DEFAULT_FISH),
    _zFar(Z_FAR),
    _cameraPos(CAMERA_POS_DEFAULT_FISH),
    _cameraPosDis(0.f),
    _fovAngleDis(0.f),
    _zNearDis(0.f),
    _zFarDis(0.f),
    _scaleDis(0.f),
    _yRotateAngleDis(0.f),
    _animationFrameRestNum(0),
    _xRotateVelocity(0.f),
    _yRotateVelocity(0.f),
    _running(false),
    _pboBytes(nullptr),
    _pboFaceBytes(nullptr),
    _filterType(Filter_Type::NONE)
{
    if(FaceDetectManager::getSingletonPtr()==nullptr){
        new FaceDetectManager(tDetectModelPath, tAlignModelPath);
    }
    _cameraMediaCodec = new CameraMediaCodec();
    _identityM      =   glm::mat4(1.0f);
    _modelM         =   glm::mat4(1.0f);
    _scaleM         =   glm::mat4(1.0f);
    _rotateM        =   glm::mat4(1.0f);
    //_rotateM        =   glm::rotate(glm::radians(_xRotateAngle), glm::vec3(0, 1, 0));
    _mvpTakePhotoM  =   glm::mat4(1.0f);
    _sphereData     =   new SphereData();
    updateViewMode(ViewMode::FISH);
}
CameraVideoRender::~CameraVideoRender() {
    _pboBytes = nullptr;
    _renderCallbackObj = nullptr;
    nativeDisConnect();
    deInitGL();
    if(_sphereData != nullptr){
        delete _sphereData;
        _sphereData = nullptr;
    }
    if(_cameraMediaCodec != nullptr){
        delete _cameraMediaCodec;
        _cameraMediaCodec = nullptr;
    }
    if(_lutTextureRGB != nullptr){
        delete[] _lutTextureRGB;
        _lutTextureRGB  =   nullptr;
    }
}
void CameraVideoRender::nativeOnCreate(const char* mediaDir, int sphereVertexNum, float radius, AAssetManager* aAssetManager){
    _mediaDir               =   mediaDir;
    _aAssetManager          =   aAssetManager;
    _sphereData->init(sphereVertexNum, radius);
}
void CameraVideoRender::nativeInitGL(float recordWidth, float recordHeight, int takePhotoWidth, int takePhotoHeight, jobject render_callback_obj, jmethodID callbackTakePhotoMethod, int timeScale, int frameRate){
    _recordWidth                =   recordWidth;
    _recordHeight               =   recordHeight;
    _takePhotoWidth             =   takePhotoWidth;
    _takePhotoHeight            =   takePhotoHeight;
    _renderCallbackObj          =   render_callback_obj;
    _callbackTakePhotoMethod    =   callbackTakePhotoMethod;
    if(CompileShaderProgram(filter_camera_position_vert, filter_camera_play_frag, &_sFilterProgramPlay) && CompileShaderProgram(panorama_position_vert, panorama_play_frag, &_sProgramPlay) && CompileShaderProgram(panorama_face_position_vert, panorama_face_play_frag, &_sFaceProgramPlay)){
        _aFaceVertexPosHandlePlay          = glGetAttribLocation(_sFaceProgramPlay,    "a_VertexPos");
        _aFaceVertexUVHandlePlay           = glGetAttribLocation(_sFaceProgramPlay,    "a_VertexUV");
        _uFaceMvpMatrixLocPlay             = glGetUniformLocation(_sFaceProgramPlay,   "u_MvpMatrix");
        _uFaceCameraTextureLocPlay         = glGetUniformLocation(_sFaceProgramPlay,   "u_CameraTextureOES");

        _aFilterVertexPosHandlePlay         = glGetAttribLocation(_sFilterProgramPlay,    "a_VertexPos");
        _aFilterVertexUVHandlePlay          = glGetAttribLocation(_sFilterProgramPlay,    "a_VertexUV");
        _uFilterMvpMatrixLocPlay            = glGetUniformLocation(_sFilterProgramPlay,   "u_MvpMatrix");
        _uFilterCameraTextureLocPlay        = glGetUniformLocation(_sFilterProgramPlay,   "u_CameraTextureOES");
        _uFilterLutSizeLocPlay              = glGetUniformLocation(_sFilterProgramPlay,   "u_LutSize");
        _uFilterLutTextureLocPlay           = glGetUniformLocation(_sFilterProgramPlay,   "u_LutTexture");
        _uFilterLogoAngleLocPlay            = glGetUniformLocation(_sFilterProgramPlay,   "u_LogoAngle");
        _uFilterLogoTextureLocPlay          = glGetUniformLocation(_sFilterProgramPlay,   "u_LogoTexture");
        _uFilterGpuImageLocPlay             = glGetUniformLocation(_sFilterProgramPlay,   "u_GpuImage");

        _uFilterEffectLatitudeLocPlay       = glGetUniformLocation(_sFilterProgramPlay,   "u_Latitude");
        _uFilterEffectLongitudeLocPlay      = glGetUniformLocation(_sFilterProgramPlay,   "u_Longitude");
        _uFilterEffectFaceSizeLocPlay       = glGetUniformLocation(_sFilterProgramPlay,   "u_FaceSize");

        _aVertexPosHandlePlay               = glGetAttribLocation(_sProgramPlay,          "a_VertexPos");
        _aVertexUVHandlePlay                = glGetAttribLocation(_sProgramPlay,          "a_VertexUV");
        _uMvpMatrixLocPlay                  = glGetUniformLocation(_sProgramPlay,         "u_MvpMatrix");
        _uCameraTextureLocPlay              = glGetUniformLocation(_sProgramPlay,         "u_CameraTextureOES");
        _uLutTextureLocPlay                 = glGetUniformLocation(_sProgramPlay,         "u_LutTexture");
        _uLutSizeLocPlay                    = glGetUniformLocation(_sProgramPlay,         "u_LutSize");
        _uLogoTextureLocPlay                = glGetUniformLocation(_sProgramPlay,         "u_LogoTexture");
        _uLogoAngleLocPlay                  = glGetUniformLocation(_sProgramPlay,         "u_LogoAngle");
        _uGpuImageLocPlay                   = glGetUniformLocation(_sProgramPlay,         "u_GpuImage");

        _uEffectLatitudeLocPlay             = glGetUniformLocation(_sProgramPlay,         "u_Latitude");
        _uEffectLongitudeLocPlay            = glGetUniformLocation(_sProgramPlay,         "u_Longitude");
        _uEffectFaceSizeLocPlay             = glGetUniformLocation(_sProgramPlay,         "u_FaceSize");

        glGenBuffers(2, _vboBuffers);
        glBindBuffer(GL_ARRAY_BUFFER, _vboBuffers[0]);
        glBufferData(GL_ARRAY_BUFFER, _sphereData->_size*20, _sphereData->_vertexUVBuff, GL_STATIC_DRAW );
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, _vboBuffers[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(_takePhotoVUV), _takePhotoVUV, GL_STATIC_DRAW );
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenTextures(4, _textures);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, _textures[0]);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _textures[1]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, _textures[2]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenFramebuffers(2, _frameBuffers);
        glGenBuffers(2, _pboIdsBuffers);
        _cameraMediaCodec->post(CAMERA_MEDIA_CODEC_MSG_TYPE::kMsgInitGL, true);
        CheckOpenGL("mmmm2");
    }else{
        LOGE("CompileShaderProgram");
    }
}
void CameraVideoRender::nativeOnSurfaceChanged(float pNewWidth, float pNewHeight){
    _widgetWidth            =   pNewWidth;
    _widgetHeight           =   pNewHeight;
    _faceWidth              =   _widgetWidth*0.5f;
    _faceHeight             =   _widgetHeight*0.5f;
    _faceCalWidth           =   _faceWidth*0.5f;
    _faceCalHeight          =   _faceHeight*0.5f;
    _diagonalLength         =   sqrt(_widgetWidth*_widgetWidth + _widgetHeight*_widgetHeight);
}
void CameraVideoRender::deInitGL(){
    glDeleteTextures(4, _textures);
    glDeleteBuffers(2, _vboBuffers);
    glDeleteBuffers(2, _pboIdsBuffers );
    if(_sProgramPlay){
     	glDeleteProgram(_sProgramPlay);
     	_sProgramPlay = 0;
    }
    if(_sFilterProgramPlay){
        glDeleteProgram(_sFilterProgramPlay);
        _sFilterProgramPlay = 0;
    }
    glDeleteFramebuffers(2, _frameBuffers);
    _lutSize    =   0;
}
GLint CameraVideoRender::nativeGetTextureId(){
    return _textures[0];
}
void CameraVideoRender::drawRecordFilterFrame(){
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, _recordWidth, _recordHeight);
    glUseProgram(_sFilterProgramPlay);
    glUniformMatrix4fv(_uFilterMvpMatrixLocPlay, 1, false, glm::value_ptr(_mvpTakePhotoM));
    glUniform1i(_uFilterCameraTextureLocPlay, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, _textures[0]);
    //filter
    glUniform1f(_uFilterLutSizeLocPlay, (float)_lutSize);
    glUniform1f(_uFilterEffectLatitudeLocPlay, _latitude);
    glUniform1f(_uFilterEffectLongitudeLocPlay, _longitude);
    glUniform1f(_uFilterEffectFaceSizeLocPlay, _faceSize);
    if(_lutSize > 0.f && _lutTextureRGB != nullptr){
        glUniform1i(_uFilterLutTextureLocPlay, 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _textures[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _lutSize*_lutSize, _lutSize, 0, GL_RGB, GL_UNSIGNED_BYTE, _lutTextureRGB);
    }else{
        if(_faceSize>0.f){
            glUniform1i(_uFilterLutTextureLocPlay, 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, _textures[1]);
            auto effectPixelFormat = _effectImage.getRenderFormat();
            switch(effectPixelFormat){
                case PixelFormat::BGRA8888:    //! 32-bit texture: BGRA8888
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_BGRA, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                case PixelFormat::RGBA8888:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                case PixelFormat::RGB888:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                case PixelFormat::RGB565:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, _effectImage.getData());
                    break;
                case PixelFormat::RGBA4444:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, _effectImage.getData());
                    break;
                case PixelFormat::RGB5A1:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, _effectImage.getData());
                    break;
                case PixelFormat::A8:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_ALPHA, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                case PixelFormat::I8:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                case PixelFormat::AI88:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                default:
                    LOGE("＝＝＝＝＝＝ERROR !! FORMAT %d", (int)effectPixelFormat);
                    break;
            }
        }else{
            glUniform1i(_uFilterLutTextureLocPlay, 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, _textures[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _lutSize*_lutSize, _lutSize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        }
    }
    glUniform1f(_uFilterLogoAngleLocPlay, 1.0f-_logoAngle/180.f);
    glUniform1i(_uFilterGpuImageLocPlay, (int)_filterType);
    glUniform1i(_uFilterLogoTextureLocPlay, 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _textures[2]);
    auto logoPixelFormat = _logoImage.getRenderFormat();
    switch(logoPixelFormat){
        case PixelFormat::BGRA8888:    //! 32-bit texture: BGRA8888
            glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_BGRA, GL_UNSIGNED_BYTE, _logoImage.getData());
            break;
        case PixelFormat::RGBA8888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, _logoImage.getData());
            break;
        case PixelFormat::RGB888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, _logoImage.getData());
            break;
        case PixelFormat::RGB565:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, _logoImage.getData());
            break;
        case PixelFormat::RGBA4444:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, _logoImage.getData());
            break;
        case PixelFormat::RGB5A1:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, _logoImage.getData());
            break;
        case PixelFormat::A8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_ALPHA, GL_UNSIGNED_BYTE, _logoImage.getData());
            break;
        case PixelFormat::I8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, _logoImage.getData());
            break;
        case PixelFormat::AI88:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, _logoImage.getData());
            break;
        default:
            LOGE("ERROR !! FORMAT %d", (int)logoPixelFormat);
            break;
    }
    glBindBuffer(GL_ARRAY_BUFFER, _vboBuffers[1]);
    glEnableVertexAttribArray(_aFilterVertexPosHandlePlay);
    glVertexAttribPointer(
        _aFilterVertexPosHandlePlay,
        3,
        GL_FLOAT,
        GL_FALSE,
        20,
        (void*)0
    );
    glEnableVertexAttribArray(_aFilterVertexUVHandlePlay);
    glVertexAttribPointer(
        _aFilterVertexUVHandlePlay,
        2,
        GL_FLOAT,
        GL_FALSE,
        20,
        (void*)12
    );
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(_aFilterVertexPosHandlePlay);
    glDisableVertexAttribArray(_aFilterVertexUVHandlePlay);
    CheckOpenGL("mmmm1");
}
void CameraVideoRender::drawFilterFrame(){
     if(_takePhotoTickCount == 0 || _pboBytes!=nullptr){
         return;
     }
     _takePhotoTickCount = 0;
     glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffers[0]);
     glBindTexture(GL_TEXTURE_2D, _textures[3]);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _takePhotoWidth, _takePhotoHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
     //glBindTexture(GL_TEXTURE_2D, _textures[3]);
     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _textures[3], 0);
     glBindBuffer(GL_PIXEL_PACK_BUFFER, _pboIdsBuffers[0]);
     glBufferData(GL_PIXEL_PACK_BUFFER, _takePhotoWidth*_takePhotoHeight*4, nullptr, GL_DYNAMIC_READ);
     glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
     glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
     glViewport(0, 0, _takePhotoWidth, _takePhotoHeight);
     glUseProgram(_sFilterProgramPlay);
     glUniformMatrix4fv(_uFilterMvpMatrixLocPlay, 1, false, glm::value_ptr(_mvpTakePhotoM));
     glUniform1i(_uFilterCameraTextureLocPlay, 0);
     glActiveTexture(GL_TEXTURE0);
     glBindTexture(GL_TEXTURE_EXTERNAL_OES, _textures[0]);
     //filter
     glUniform1f(_uFilterLutSizeLocPlay, (float)_lutSize);
     glUniform1f(_uFilterEffectLatitudeLocPlay, _latitude);
     glUniform1f(_uFilterEffectLongitudeLocPlay, _longitude);
     glUniform1f(_uFilterEffectFaceSizeLocPlay, _faceSize);
     if(_lutSize > 0.f && _lutTextureRGB != nullptr){
         glUniform1i(_uFilterLutTextureLocPlay, 1);
         glActiveTexture(GL_TEXTURE1);
         glBindTexture(GL_TEXTURE_2D, _textures[1]);
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _lutSize*_lutSize, _lutSize, 0, GL_RGB, GL_UNSIGNED_BYTE, _lutTextureRGB);
     }else{
        if(_faceSize>0.f){
            glUniform1i(_uFilterLutTextureLocPlay, 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, _textures[1]);
            auto effectPixelFormat = _effectImage.getRenderFormat();
            switch(effectPixelFormat){
                case PixelFormat::BGRA8888:    //! 32-bit texture: BGRA8888
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_BGRA, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                case PixelFormat::RGBA8888:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                case PixelFormat::RGB888:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                case PixelFormat::RGB565:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, _effectImage.getData());
                    break;
                case PixelFormat::RGBA4444:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, _effectImage.getData());
                    break;
                case PixelFormat::RGB5A1:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, _effectImage.getData());
                    break;
                case PixelFormat::A8:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_ALPHA, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                case PixelFormat::I8:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                case PixelFormat::AI88:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                default:
                    LOGE("ERROR !! FORMAT %d", (int)effectPixelFormat);
                    break;
            }
        }else{
             glUniform1i(_uFilterLutTextureLocPlay, 1);
             glActiveTexture(GL_TEXTURE1);
             glBindTexture(GL_TEXTURE_2D, _textures[1]);
             glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _lutSize*_lutSize, _lutSize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        }
     }
     glUniform1f(_uFilterLogoAngleLocPlay, 1.0f-_logoAngle/180.f);
     glUniform1i(_uFilterGpuImageLocPlay, (int)_filterType);
     glUniform1i(_uFilterLogoTextureLocPlay, 2);
     glActiveTexture(GL_TEXTURE2);
     glBindTexture(GL_TEXTURE_2D, _textures[2]);
     auto logoPixelFormat = _logoImage.getRenderFormat();
     switch(logoPixelFormat){
         case PixelFormat::BGRA8888:    //! 32-bit texture: BGRA8888
             glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_BGRA, GL_UNSIGNED_BYTE, _logoImage.getData());
             break;
         case PixelFormat::RGBA8888:
             glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, _logoImage.getData());
             break;
         case PixelFormat::RGB888:
             glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, _logoImage.getData());
             break;
         case PixelFormat::RGB565:
             glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, _logoImage.getData());
             break;
         case PixelFormat::RGBA4444:
             glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, _logoImage.getData());
             break;
         case PixelFormat::RGB5A1:
             glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, _logoImage.getData());
             break;
         case PixelFormat::A8:
             glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_ALPHA, GL_UNSIGNED_BYTE, _logoImage.getData());
             break;
         case PixelFormat::I8:
             glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, _logoImage.getData());
             break;
         case PixelFormat::AI88:
             glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, _logoImage.getData());
             break;
         default:
             LOGE("ERROR !! FORMAT %d", (int)logoPixelFormat);
             break;
     }
     glBindBuffer(GL_ARRAY_BUFFER, _vboBuffers[1]);
     glEnableVertexAttribArray(_aFilterVertexPosHandlePlay);
     glVertexAttribPointer(
         _aFilterVertexPosHandlePlay,
         3,
         GL_FLOAT,
         GL_FALSE,
         20,
         (void*)0
     );
     glEnableVertexAttribArray(_aFilterVertexUVHandlePlay);
     glVertexAttribPointer(
         _aFilterVertexUVHandlePlay,
         2,
         GL_FLOAT,
         GL_FALSE,
         20,
         (void*)12
     );
     glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
     glDisableVertexAttribArray(_aFilterVertexPosHandlePlay);
     glDisableVertexAttribArray(_aFilterVertexUVHandlePlay);
     glReadBuffer(GL_COLOR_ATTACHMENT0);
     glBindBuffer(GL_PIXEL_PACK_BUFFER, _pboIdsBuffers[0]);
     glReadPixels(0, 0, _takePhotoWidth, _takePhotoHeight, GL_RGBA, GL_UNSIGNED_BYTE, 0);
     glBindBuffer(GL_PIXEL_PACK_BUFFER, _pboIdsBuffers[0]);
     _pboBytes = (GLubyte*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, _takePhotoWidth*_takePhotoHeight*4, GL_MAP_READ_BIT);
     if (_pboBytes!=nullptr){
         WorkerData* _workerData = new WorkerData(_pboBytes, _takePhotoWidth, _takePhotoHeight, _takePhotoWidth*_takePhotoHeight*4, _mediaDir.c_str());
         post(TaskMessageType::kMsgSaveData, _workerData);
     }
     glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
     glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
     glBindFramebuffer(GL_FRAMEBUFFER, 0);
     CheckOpenGL("mmm111m2");
 }
void CameraVideoRender::drawPanoramaFrame(){
    doAnimation();
    glClear(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    _viewM = glm::lookAt(
        glm::vec3(0,0,_cameraPos),
        glm::vec3(0,0,-1),
        glm::vec3(0,1,0)
    );
    _projectionM = glm::perspective(glm::radians(_fovAngle),(float)_widgetWidth/(float)_widgetHeight,_zNear, _zFar);
    _scaleM = glm::scale(glm::mat4(1.0f), glm::vec3(_scale));
    _mvpM = _projectionM * _viewM * _rotateM * _scaleM;
    glViewport(0,0,_widgetWidth, _widgetHeight);
    glUseProgram(_sProgramPlay);
    glUniformMatrix4fv(_uMvpMatrixLocPlay, 1, false,glm::value_ptr(_mvpM));
    glBindBuffer(GL_ARRAY_BUFFER, _vboBuffers[0]);
    glEnableVertexAttribArray(_aVertexPosHandlePlay);
    glVertexAttribPointer(
        _aVertexPosHandlePlay,
        3,
        GL_FLOAT,
        GL_FALSE,
        20,
        (void*)0
    );
    glEnableVertexAttribArray(_aVertexUVHandlePlay);
    glVertexAttribPointer(
        _aVertexUVHandlePlay,
        2,
        GL_FLOAT,
        GL_FALSE,
        20,
        (void*)12
    );
    glUniform1i(_uCameraTextureLocPlay, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, _textures[0]);

    glUniform1f(_uEffectLatitudeLocPlay, _latitude);
    glUniform1f(_uEffectLongitudeLocPlay, _longitude);
    glUniform1f(_uEffectFaceSizeLocPlay, _faceSize);
    glUniform1f(_uLutSizeLocPlay, (float)_lutSize);
    if (_lutSize > 0){
        glUniform1i(_uLutTextureLocPlay, 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _textures[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _lutSize*_lutSize, _lutSize, 0, GL_RGB, GL_UNSIGNED_BYTE, _lutTextureRGB);
    }else{
        if(_faceSize>0.f){
            glUniform1i(_uLutTextureLocPlay, 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, _textures[1]);
            auto effectPixelFormat = _effectImage.getRenderFormat();
            switch(effectPixelFormat){
                case PixelFormat::BGRA8888:    //! 32-bit texture: BGRA8888
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_BGRA, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                case PixelFormat::RGBA8888:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                case PixelFormat::RGB888:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                case PixelFormat::RGB565:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, _effectImage.getData());
                    break;
                case PixelFormat::RGBA4444:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, _effectImage.getData());
                    break;
                case PixelFormat::RGB5A1:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, _effectImage.getData());
                    break;
                case PixelFormat::A8:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_ALPHA, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                case PixelFormat::I8:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                case PixelFormat::AI88:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, _effectImage.getWidth(), _effectImage.getHeight(), 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, _effectImage.getData());
                    break;
                default:
                    LOGE("ERROR !! FORMAT %d", (int)effectPixelFormat);
                    break;
            }
        }else{
            glUniform1f(_uLutSizeLocPlay, 0.0);
            glUniform1i(_uLutTextureLocPlay, 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, _textures[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _lutSize*_lutSize, _lutSize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        }
    }
    glUniform1f(_uLogoAngleLocPlay, 1.0f-_logoAngle/180.f);
    glUniform1i(_uGpuImageLocPlay, (int)_filterType);
    glUniform1i(_uLogoTextureLocPlay, 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _textures[2]);
    auto logoPixelFormat = _logoImage.getRenderFormat();
    switch(logoPixelFormat){
        case PixelFormat::BGRA8888:    //! 32-bit texture: BGRA8888
            glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_BGRA, GL_UNSIGNED_BYTE, _logoImage.getData());
            break;
        case PixelFormat::RGBA8888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, _logoImage.getData());
            break;
        case PixelFormat::RGB888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, _logoImage.getData());
            break;
        case PixelFormat::RGB565:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, _logoImage.getData());
            break;
        case PixelFormat::RGBA4444:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, _logoImage.getData());
            break;
        case PixelFormat::RGB5A1:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, _logoImage.getData());
            break;
        case PixelFormat::A8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_ALPHA, GL_UNSIGNED_BYTE, _logoImage.getData());
            break;
        case PixelFormat::I8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, _logoImage.getData());
            break;
        case PixelFormat::AI88:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, _logoImage.getWidth(), _logoImage.getHeight(), 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, _logoImage.getData());
            break;
        default:
            LOGE("ERROR !! FORMAT %d", (int)logoPixelFormat);
            break;
    }
    glDrawArrays(GL_TRIANGLES, 0, _sphereData->_size);
    glDisableVertexAttribArray(_aVertexPosHandlePlay);
    glDisableVertexAttribArray(_aVertexUVHandlePlay);
    drawFaceFrame();
    CheckOpenGL("mmmm2222");
}
void CameraVideoRender::drawFaceFrame(){
    //LOGE("pppp%d,%d", _useFaceDetector, _startFace);
    if(_useFaceDetector == 0){
        //_latitude = 0.0f;
        //_longitude = 0.0f;
        //_faceSize = 0.0f;
        return;
    }
    if(_startFace >= 0 || _pboFaceBytes!=nullptr){
        _startFace -- ;
        return;
    }
    _cameraFacePos = _cameraPos;
    _faceFovAngle = _fovAngle;
    _faceScaleM   = _scaleM;
    _faceRotateM  = _rotateM;
    _zFaceNear    = _zNear;
    _zFaceFar     = _zFar;
    _projectionFaceM = glm::perspective(glm::radians(_faceFovAngle),(float)_faceWidth/(float)_faceHeight,_zFaceNear, _zFaceFar);
    _viewFaceM      = _viewM;
    _mvpFaceM       = _mvpM;
    glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffers[1]);
    glBindTexture(GL_TEXTURE_2D, _textures[3]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _faceWidth, _faceHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, _textures[3]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _textures[3], 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, _pboIdsBuffers[1]);
    glBufferData(GL_PIXEL_PACK_BUFFER, _faceWidth*_faceHeight*4, nullptr, GL_DYNAMIC_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0,_faceWidth, _faceHeight);
    glUseProgram(_sFaceProgramPlay);
    glUniformMatrix4fv(_uFaceMvpMatrixLocPlay, 1, false,glm::value_ptr(_mvpFaceM));
    glBindBuffer(GL_ARRAY_BUFFER, _vboBuffers[0]);
    glEnableVertexAttribArray(_aFaceVertexPosHandlePlay);
    glVertexAttribPointer(
        _aFaceVertexPosHandlePlay,
        3,
        GL_FLOAT,
        GL_FALSE,
        20,
        (void*)0
    );
    glEnableVertexAttribArray(_aFaceVertexUVHandlePlay);
    glVertexAttribPointer(
        _aFaceVertexUVHandlePlay,
        2,
        GL_FLOAT,
        GL_FALSE,
        20,
        (void*)12
    );
    glUniform1i(_uFaceCameraTextureLocPlay, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, _textures[0]);
    glDrawArrays(GL_TRIANGLES, 0, _sphereData->_size);
    glDisableVertexAttribArray(_aFaceVertexPosHandlePlay);
    glDisableVertexAttribArray(_aFaceVertexUVHandlePlay);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, _pboIdsBuffers[1]);
    glReadPixels(_faceCalWidth*0.5, _faceCalHeight*0.5, _faceCalWidth, _faceCalHeight, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, _pboIdsBuffers[1]);
    _pboFaceBytes = (GLubyte*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, _faceCalWidth*_faceCalHeight*4, GL_MAP_READ_BIT);
    if (_pboFaceBytes!=nullptr){
        FaceData* faceData = new FaceData(_pboFaceBytes, _faceCalWidth, _faceCalHeight);
        postFace(FaceMessageType::kMsgFaceDetectorData, faceData);
    }
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CheckOpenGL("mmm3333m2");
}
void CameraVideoRender::nativeDrawPanoramaFrame(){
    drawRecordFilterFrame();
}
void CameraVideoRender::nativeDrawFrame(){
    if(!_cameraMediaCodec->isHadIFrame()){
        return;
    }
    drawFilterFrame();
    drawPanoramaFrame();
}
void CameraVideoRender::nativeSetSurface(ANativeWindow* aNativeWindow){
    if(_cameraMediaCodec!=nullptr){
        CameraMediaCodecData *cameraMediaCodecData = new CameraMediaCodecData();
        cameraMediaCodecData->_aNativeWindow = aNativeWindow;
        CameraMediaCodecMsg *msg = new CameraMediaCodecMsg();
        msg->what = CAMERA_MEDIA_CODEC_MSG_TYPE::kMsgUpdateSurface;
        msg->obj = cameraMediaCodecData;
        msg->next = nullptr;
        msg->quit = false;
        _cameraMediaCodec->addMsg(msg, true);
    }
}
void CameraVideoRender::nativeCallBackFaceDetector(int width, int height, float x, float y, float eyesDistance){
    if(eyesDistance <=0.f){
        return;
    }
    auto mouseX = x / (width  * 0.5f) - 1.0f;
    auto mouseY = y / (height * 0.5f) - 1.0f;
    auto invVP = glm::inverse(_projectionFaceM * _viewFaceM);
    auto screenPos = glm::vec4(mouseX, -mouseY, 1.0f, 1.0f);
    auto worldPos = invVP * screenPos;
    auto rayDir = glm::normalize(glm::vec3(worldPos));
    auto rayOrigin = glm::vec3(0.f, 0.f, _cameraFacePos); // some camera position, this is supplied by you
    auto sphereCenter = glm::vec3(0.f, 0.f, 0.f);
    auto sphereRadius = _scale;
    std::vector<glm::vec3> intersectPoint;
    if(getRaySphereIntersect(rayOrigin, rayDir, sphereCenter, sphereRadius, intersectPoint)){
        glm::vec3 start = intersectPoint[0];
        glm::vec4 Position = glm::vec4( start, 1.0 );
        glm::vec4 Transformed = glm::inverse(_rotateM) * Position;
        auto latitude = acos(Transformed.y/sphereRadius);
        auto longitude = atan(Transformed.z/Transformed.x);
        if(Transformed.x>=0.f){
            longitude = M_PI*0.5f - longitude;
        }else{
            longitude = M_PI*1.5f - longitude;
        }
        //LOGE("OOPPPP %f,%f,%d,%d", x, y, width, height);
        //LOGE("OOPPPP %f,%f,%f", start.x, start.y, start.z);
        //LOGE("OOPPPP %f,%f,%f", Transformed.x, Transformed.y, Transformed.z);
        //LOGE("KKLL%f,%f", latitude, longitude);
        _latitude = latitude/M_PI;
        _longitude = 1.0f-longitude/M_PI*0.5f;
        //LOGE("KKLL%f,%f", _latitude, _longitude);
        //LOGE("========%f,%f", _yRotateAngle, _xRotateAngle);
        _faceSize = (eyesDistance+10)/width;
    }else{
        _latitude = 0.0f;
        _longitude = 0.0f;
        _faceSize = 0.0f;
    }
}
bool CameraVideoRender::getRaySphereIntersect(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 sphereCenter, float sphereRadius, std::vector<glm::vec3> &intersectPoint){
    auto v = rayOrigin - sphereCenter;
    auto b = 2.0f * glm::dot(rayDir, v);
    auto c = glm::dot(v, v) - sphereRadius * sphereRadius;
    auto discriminant = (b * b) - (4.0f * c);
    if (discriminant < 0.0f){
        return false;
    }
    discriminant = sqrt(discriminant);
    auto far = (-b + discriminant) / 2.0f;
    auto near = (-b - discriminant) / 2.0f;
    auto intersectFarPoint = rayOrigin + rayDir*far;
    auto intersectNearPoint = rayOrigin + rayDir*near;
    auto res =  (far >= 0.0f || near >= 0.0f);
    if (res){
        if (near > 0 ){
            intersectPoint.push_back(intersectNearPoint);
        }
        if (far > 0){
            intersectPoint.push_back(intersectFarPoint);
        }
    }
    return res;
}
void CameraVideoRender::updateLutFilter(AAssetManager* mgr, const char* filePath, int filterType, int useFaceDetector){
    _latitude = 0.0f;
    _longitude = 0.0f;
    _faceSize = 0.0f;
    if(useFaceDetector == 1){
        _lutSize    =   0;
        _filterType = Filter_Type::NONE;
        updateEffect(filePath, useFaceDetector);
    }else{
        _useFaceDetector = 0;
        _startFace = 0;
        if(filePath == nullptr){
            _lutSize    =   0;
        }else{
            if(_lutTextureRGB != nullptr){
                delete[] _lutTextureRGB;
                _lutTextureRGB  =   nullptr;
            }
            _lutTextureRGB  =   LutUtil::loadLutFile(mgr, filePath, &_lutSize);
            if(_lutTextureRGB == nullptr){
                _lutSize    =   0;
            }
        }
        _filterType = (Filter_Type)filterType;
    }
}
void CameraVideoRender::updateEffect(std::string effectFilePath, int useFaceDetector){
    _effectFilePath  = effectFilePath;
    _useFaceDetector = useFaceDetector;
     AAsset* asset 						= AAssetManager_open(_aAssetManager, _effectFilePath.c_str(), AASSET_MODE_UNKNOWN);
     if(asset != nullptr){
         auto fileSize 				    = AAsset_getLength(asset);
         auto imageData 			    = new unsigned char[fileSize];
         auto unpackedLen  				= AAsset_read(asset, (void*)imageData, fileSize);
         if(imageData == nullptr){
             _effectImage.clear();
         }else{
             _effectImage.initWithImageData(imageData, unpackedLen);
             if(_useFaceDetector == 1){
                _startFace = FACE_FRAME_RATIO;
             }
         }
         AAsset_close(asset);
     }else{
         LOGE("read_JPEG_file%s failed!", _effectFilePath.c_str());
     }
}
void CameraVideoRender::updateScale(float scale){
    switch(_viewMode){
        case ViewMode::FISH:{
            _scale = _scale + scale/_diagonalLength*10.f;
            if(_scale >= SCALE_MAX_FISH){
                _scale = SCALE_MAX_FISH;
            }else if(_scale <= SCALE_MIN_FISH){
                _scale = SCALE_MIN_FISH;
            }
        }
            break;
        case ViewMode::PERSPECTIVE:{
            _fovAngle = _fovAngle - scale/_diagonalLength*100.f;
             if(_fovAngle >= FOV_MAX_DEFAULT_PERSPECTIVE){
                 _fovAngle = FOV_MAX_DEFAULT_PERSPECTIVE;
             }else if(_fovAngle <= FOV_MIN_DEFAULT_PERSPECTIVE){
                 _fovAngle = FOV_MIN_DEFAULT_PERSPECTIVE;
             }
        }
            break;
        case ViewMode::PLANET:{
            _scale = _scale + scale/_diagonalLength*10.f;
            if(_scale >= SCALE_MAX_PLANET){
                _scale = SCALE_MAX_PLANET;
            }else if(_scale <= SCALE_MIN_PLANET){
                _scale = SCALE_MIN_PLANET;
            }
        }
            break;
        case ViewMode::CRYSTAL_BALL:{
            _scale = _scale + scale/_diagonalLength*10.f;
            if(_scale >= SCALE_MAX_FISH){
                _scale = SCALE_MAX_FISH;
            }else if(_scale <= SCALE_MIN_FISH){
                _scale = SCALE_MIN_FISH;
            }
        }
            break;
    }
}

void CameraVideoRender::updateLogoAngle(float angle){
    _logoAngle = angle;
}

void CameraVideoRender::updateRotate(float xMove, float yMove){
    if(_animationFrameRestNum > 0){
        return;
    }
    _yRotateAngle   +=  yMove;
    if(_yRotateAngle>=Y_ROTATE_ANGLE){
        _yRotateAngle   =   Y_ROTATE_ANGLE;
    }else if(_yRotateAngle<=-Y_ROTATE_ANGLE){
        _yRotateAngle   =   -Y_ROTATE_ANGLE;
    }
    _xRotateAngle   +=  xMove;
    _rotateM        =   glm::rotate(glm::radians(_yRotateAngle), glm::vec3(1, 0, 0))*glm::rotate(glm::radians(_xRotateAngle), glm::vec3(0, 1, 0));
}
void CameraVideoRender::updateViewMode(ViewMode viewMode){
    _viewMode = viewMode;
    switch(_viewMode){
        case ViewMode::FISH:{
            _cameraPosDis      = (CAMERA_POS_DEFAULT_FISH-_cameraPos)*ANIMATION_FRAME_TIME;
            _fovAngleDis       = (FOV_DEFAULT_FISH-_fovAngle)*ANIMATION_FRAME_TIME;
            _zNearDis          = (Z_NEAR_DEFAULT_FISH-_zNear)*ANIMATION_FRAME_TIME;
            _zFarDis           = (Z_FAR-_zFar)*ANIMATION_FRAME_TIME;
            _scaleDis          = (SCALE_DEFAULT_FISH-_scale)*ANIMATION_FRAME_TIME;
            _yRotateAngleDis   = (ROTATE_ANGLE_Y_DEFAULT_FISH-_yRotateAngle)*ANIMATION_FRAME_TIME;
            _animationFrameRestNum = ANIMATION_FRAME_NUM;
        }
            break;
        case ViewMode::PERSPECTIVE:{
            _cameraPosDis      = (CAMERA_POS_DEFAULT_PERSPECTIVE-_cameraPos)*ANIMATION_FRAME_TIME;
            _fovAngleDis       = (FOV_DEFAULT_PERSPECTIVE-_fovAngle)*ANIMATION_FRAME_TIME;
            _zNearDis          = (Z_NEAR_DEFAULT_PERSPECTIVE-_zNear)*ANIMATION_FRAME_TIME;
            _zFarDis           = (Z_FAR-_zFar)*ANIMATION_FRAME_TIME;
            _scaleDis          = (SCALE_DEFAULT_PERSPECTIVE-_scale)*ANIMATION_FRAME_TIME;
            _yRotateAngleDis   = (ROTATE_ANGLE_Y_DEFAULT_PERSPECTIVE-_yRotateAngle)*ANIMATION_FRAME_TIME;
            _animationFrameRestNum = ANIMATION_FRAME_NUM;
        }
            break;
        case ViewMode::PLANET:{
            _cameraPosDis      = (CAMERA_POS_DEFAULT_PLANET-_cameraPos)*ANIMATION_FRAME_TIME;
            _fovAngleDis       = (FOV_DEFAULT_PLANET-_fovAngle)*ANIMATION_FRAME_TIME;
            _zNearDis          = (Z_NEAR_DEFAULT_PLANET-_zNear)*ANIMATION_FRAME_TIME;
            _zFarDis           = (Z_FAR-_zFar)*ANIMATION_FRAME_TIME;
            _scaleDis          = (SCALE_DEFAULT_PLANET-_scale)*ANIMATION_FRAME_TIME;
            _yRotateAngleDis   = (ROTATE_ANGLE_Y_DEFAULT_PLANET-_yRotateAngle)*ANIMATION_FRAME_TIME;
            _animationFrameRestNum = ANIMATION_FRAME_NUM;
        }
            break;
        case ViewMode::CRYSTAL_BALL:{
            _cameraPosDis      = (CAMERA_POS_DEFAULT_CRYSTAL_BALL-_cameraPos)*ANIMATION_FRAME_TIME;
            _fovAngleDis       = (FOV_DEFAULT_CRYSTAL_BALL-_fovAngle)*ANIMATION_FRAME_TIME;
            _zNearDis          = (Z_NEAR_DEFAULT_CRYSTAL_BALL-_zNear)*ANIMATION_FRAME_TIME;
            _zFarDis           = (Z_FAR-_zFar)*ANIMATION_FRAME_TIME;
            _scaleDis          = (SCALE_DEFAULT_CRYSTAL_BALL-_scale)*ANIMATION_FRAME_TIME;
            _yRotateAngleDis   = (ROTATE_ANGLE_Y_DEFAULT_CRYSTAL_BALL-_yRotateAngle)*ANIMATION_FRAME_TIME;
            _animationFrameRestNum = ANIMATION_FRAME_NUM;
        }
            break;
    }
}
void CameraVideoRender::doAnimation(){
    if(_animationFrameRestNum > 0){
        _animationFrameRestNum--;
        _cameraPos += _cameraPosDis;
        _fovAngle += _fovAngleDis;
        _zNear += _zNearDis;
        _zFar += _zFarDis;
        _scale += _scaleDis;
        if(_yRotateAngleDis!=0.f){
            _yRotateAngle   +=  _yRotateAngleDis;
            if(_yRotateAngle>= Y_ROTATE_ANGLE){
                _yRotateAngle   =   Y_ROTATE_ANGLE;
            }else if(_yRotateAngle<= -Y_ROTATE_ANGLE){
                _yRotateAngle   =   -Y_ROTATE_ANGLE;
            }
            _xRotateAngle   +=  0.0f;
            _rotateM        =   glm::rotate(glm::radians(_yRotateAngle), glm::vec3(1, 0, 0))*glm::rotate(glm::radians(_xRotateAngle), glm::vec3(0, 1, 0));
        }
    }
    doAnimationFling();
}
void CameraVideoRender::doAnimationFling(){
    if(_xRotateVelocity < 0.f){
        _xRotateVelocity += ACCELERATION;
        if(_xRotateVelocity>=0){
            _xRotateVelocity = 0.f;
        }
    }else if(_xRotateVelocity > 0.f){
        _xRotateVelocity -= ACCELERATION;
        if(_xRotateVelocity<=0){
            _xRotateVelocity = 0.f;
        }
    }
    if(_yRotateVelocity < 0.f){
        _yRotateVelocity += ACCELERATION;
        if(_yRotateVelocity>=0){
            _yRotateVelocity = 0.f;
        }
    }else if(_yRotateVelocity > 0.f){
        _yRotateVelocity -= ACCELERATION;
        if(_yRotateVelocity<=0){
            _yRotateVelocity = 0.f;
        }
    }
    if(_xRotateVelocity!=0.f || _yRotateVelocity!=0.f){
        updateRotate(_xRotateVelocity, _yRotateVelocity);
    }
}

void CameraVideoRender::updateRotateFling(float velocityX, float velocityY){
    _xRotateVelocity    =   velocityX;
    _yRotateVelocity    =   velocityY;
}

void CameraVideoRender::nativeTakePhoto(int tickCount){
    TAKE_PHOTO_RESULT result;
    if(isHasEnoughSpace(_mediaDir.c_str())){
        if(_pboBytes!=nullptr || _takePhotoTickCount>0){
            result = TAKE_PHOTO_RESULT::PHOTOING;
        }else{
            result = TAKE_PHOTO_RESULT::START;
        }
    }else{
        result = TAKE_PHOTO_RESULT::NOT_SPACE;
    }
    if (_renderCallbackObj != nullptr) {
        if(result == TAKE_PHOTO_RESULT::START){
            _takePhotoTickCount = 1;
        }
        JNIEnv *env =   JniHelper::getEnv();
        jstring _pathName = env->NewStringUTF("");
        env->CallVoidMethod(_renderCallbackObj, _callbackTakePhotoMethod, (int)result, _pathName);
    }
}

void CameraVideoRender::nativeOnResume(){
    _running = true;
}
void CameraVideoRender::nativeOnPause(){
    _running = false;
}
CameraVideoRender::CODE_CONNECT_CAMERA CameraVideoRender::nativeConnect(int vid, int pid, int fd, const char *usbFs, ANativeWindow* window){
    if(video_connected == 0){
        _scaleM                 =   glm::mat4(1.0f);
        /* Initialize camera */
        char temp[128];
        sprintf(temp, "vid=0x%x;pid=0x%x;packets_per_transfer=125", vid, pid);
        //LOGE("....%s",temp);
        auto ret = mxuvc_video_init("libusb-uvc", temp, fd);
        if(ret < 0)
            return CODE_CONNECT_CAMERA::FAIL;
        /* Set video settings */
        ret = mxuvc_video_set_format(CH_MAIN, VID_FORMAT_H264_RAW);
        if(ret < 0)
            goto error;
        ret = mxuvc_video_set_resolution(CH_MAIN, 1920, 960);
        if(ret < 0)
            goto error;
        /* Register callback functions */
        ret = mxuvc_video_register_cb(CH_MAIN, CameraMediaCodec::_onFrameCallback, _cameraMediaCodec);
        if(ret < 0)
            goto error;
        /* Start streaming */
        ret = mxuvc_video_start(CH_MAIN);
        if(ret < 0)
            goto error;
        /* In case of Bulk mode: wait a few ms and force an iframe */
        usleep(50000);
        mxuvc_video_force_iframe(CH_MAIN);
        video_connected = 1;
        return CODE_CONNECT_CAMERA::SUCCESS;
    }else{
        if(0 == mxuvc_video_stop(CH_MAIN)){
            video_connected = 0;
        }
        if(0 == mxuvc_video_deinit()){
            video_connected = 0;
        }
        return CODE_CONNECT_CAMERA::FAIL;
    }
error:
    if(0 == mxuvc_video_stop(CH_MAIN)){
        video_connected = 0;
    }
    if(0 == mxuvc_video_deinit()){
        video_connected = 0;
    }
    return CODE_CONNECT_CAMERA::FAIL;
}

CameraVideoRender::CODE_DISCONNECT_CAMERA CameraVideoRender::nativeDisConnect(){
    _rotateM        =   glm::mat4(1.0f);
    //_xRotateAngle   = 171.0f;
    //_rotateM        =   glm::rotate(glm::radians(_xRotateAngle), glm::vec3(0, 1, 0));
    _cameraMediaCodec->post(CAMERA_MEDIA_CODEC_MSG_TYPE::kMsgDisConnect, true);
    if(video_connected == 1){
        if(0 == mxuvc_video_stop(CH_MAIN)){
            video_connected = 0;
        }
        if(0 == mxuvc_video_deinit()){
            video_connected = 0;
        }
        return CODE_DISCONNECT_CAMERA::SUCCESS;
    }
    return CODE_DISCONNECT_CAMERA::FAIL;
}

void CameraVideoRender::nativeLoadLogoImage(unsigned char* imageData, ssize_t unpackedLen){
    if(imageData == nullptr){
        _logoImage.clear();
    }else{
        _logoImage.initWithImageData(imageData, unpackedLen);
    }
}

void CameraVideoRender::handle(LooperMessage* looperMessage) {
    auto what = looperMessage->what;
    auto obj = looperMessage->obj;
    switch (what) {
        case TaskMessageType::kMsgSaveData:{
            if(obj->_data == nullptr){
                return;
            }
            GEN_CAMERA_PHOTO_FILENAME(_sPhotoFileName, obj->_dir.c_str(), "jpeg");
            if(_image.saveImageToJPG_FlipY(obj->_data, true, obj->_width, obj->_height, _sPhotoFileName.c_str())){
                if (_renderCallbackObj != nullptr) {
                    JNIEnv *env =   JniHelper::getEnv();
                    jstring _pathName = env->NewStringUTF(_sPhotoFileName.c_str());
                    env->CallVoidMethod(_renderCallbackObj, _callbackTakePhotoMethod, (int)TAKE_PHOTO_RESULT::COMPLETE, _pathName);
                    _pboBytes = nullptr;
                }
            }
        }
            break;
        case TaskMessageType::kMsgQuit: {
        }
            break;
    }
}

void CameraVideoRender::handleFace(FaceMessage* faceMessage) {
    auto what = faceMessage->what;
    auto obj = faceMessage->obj;
    switch (what) {
        case FaceMessageType::kMsgFaceDetectorData:{
            if(obj->_data == nullptr){
                return;
            }
            /*std::string test;
            GEN_CAMERA_PHOTO_FILENAME(test, _mediaDir.c_str(), "bmp");
            GenBmpFile_Gray(obj->_data, 32, _faceCalWidth, _faceCalHeight, test.c_str());*/
            auto result = FaceDetectManager::getSingletonPtr()->run(obj->_data, _faceCalWidth, _faceCalHeight);
            if(result.size()>0){
                auto face = result[0];
                LOGE("===========%d,%d,%d,%d,%d,%d", face.faceInfo.bbox.x, face.faceInfo.bbox.y, face.faceInfo.bbox.width, face.faceInfo.bbox.height, _faceCalWidth, _faceCalHeight);
                LOGE("===========%f,%f", face.vLandmark[0].x, face.vLandmark[0].y);
                LOGE("===========%f,%f", face.vLandmark[1].x, face.vLandmark[1].y);
                LOGE("===========%f,%f", face.vLandmark[2].x, face.vLandmark[2].y);
                LOGE("===========%f,%f", face.vLandmark[3].x, face.vLandmark[3].y);
                LOGE("===========%f,%f", face.vLandmark[4].x, face.vLandmark[4].y);
                auto woffset = (_faceWidth - _faceCalWidth)*0.5;
                auto hoffset = (_faceHeight - _faceCalHeight)*0.5;

                auto mouseX = (face.faceInfo.bbox.x + woffset +  face.faceInfo.bbox.width*0.5)/ (_faceWidth  * 0.5f) - 1.0f;
                auto mouseY = (face.faceInfo.bbox.y  + hoffset +  face.faceInfo.bbox.height*0.5)/ (_faceHeight * 0.5f) - 1.0f;
                auto invVP = glm::inverse(_projectionFaceM * _viewFaceM);
                auto screenPos = glm::vec4(mouseX, -mouseY, 1.0f, 1.0f);
                auto worldPos = invVP * screenPos;
                auto rayDir = glm::normalize(glm::vec3(worldPos));
                auto rayOrigin = glm::vec3(0.f, 0.f, _cameraFacePos); // some camera position, this is supplied by you
                auto sphereCenter = glm::vec3(0.f, 0.f, 0.f);
                auto sphereRadius = _scale;
                std::vector<glm::vec3> intersectPoint;
                auto eyesDistance = abs(face.vLandmark[4].x-face.vLandmark[3].x);
                if(getRaySphereIntersect(rayOrigin, rayDir, sphereCenter, sphereRadius, intersectPoint)){
                    glm::vec3 start = intersectPoint[0];
                    glm::vec4 Position = glm::vec4( start, 1.0 );
                    glm::vec4 Transformed = glm::inverse(_rotateM) * Position;
                    auto latitude = acos(Transformed.y/sphereRadius);
                    auto longitude = atan(Transformed.z/Transformed.x);
                    if(Transformed.x>=0.f){
                        longitude = M_PI*0.5f - longitude;
                    }else{
                        longitude = M_PI*1.5f - longitude;
                    }
                    _latitude = latitude/M_PI;
                    _longitude = 1.0f-longitude/M_PI*0.5f;
                    _faceSize = (eyesDistance+10)/_faceWidth;
                }else{
                    _latitude = 0.0f;
                    _longitude = 0.0f;
                    _faceSize = 0.0f;
                }
            }
            _pboFaceBytes = nullptr;
            _startFace = FACE_FRAME_RATIO;
            //LOGE("[[]]]]");

            //_image.saveImageToJPG_FlipY(obj->_data, true, _faceCalWidth/2, _faceCalHeight/2, test.c_str());
        }
            break;
        case FaceMessageType::kMsgQuit: {
        }
            break;
    }
}