//
// Created by DELL on 2017/3/11.
//
#pragma once
#include "glm.hpp"
#include "ext.hpp"
#include <GLES3/gl3.h>
#include "Common.h"
#include "Singleton.h"
#include "VideoMediaCodec.h"
#include "AudioMediaCodec.h"
#include "GLUtil.h"
#include "SphereData.h"
#include "android/asset_manager.h"
#include <android/asset_manager_jni.h>
#include "Image.h"
#include "TaskMessage.h"
class PanoramaVideoRender:public TaskMessage, public AudioInterface{
    public:
        PanoramaVideoRender(const char* mediaDir, jobject panorama_callback_obj, jmethodID cbScreenShotMethod, jmethodID callbackSaveFilterMethod);
        PanoramaVideoRender& operator=(const PanoramaVideoRender& ) = delete;
        PanoramaVideoRender(PanoramaVideoRender&) = delete;
        virtual ~PanoramaVideoRender();
    private:
        int                         _videoWidth;
        int                         _videoHeight;
        int                         _widgetWidth;
        int                         _widgetHeight;
        int                         _diagonalLength;
        jobject                     _panoramaVideoCallbackObj;
        jmethodID                   _callbackScreenShotMethod;
        jmethodID                   _callbackSaveFilterMethod;
        std::string                 _mediaDir;
        std::string                 _screenDir;
        int                         _screenTag;
        Image                       _image;
        SphereData*                 _sphereData;
        float                      _scale;
        float                      _fovAngle;
        float                      _zNear;
        float                      _zFar;
        float                      _cameraPos;
        float                      _yRotateAngle;
        float                      _xRotateAngle;
        float                      _vrRotateAngle;
        glm::mat4                   _identityM;
        glm::mat4                   _projectionM;
        glm::mat4                   _viewM;
        glm::mat4                   _rotateM;
        glm::mat4                   _sRotateM;
        glm::mat4                   _sVrRotateM;
        glm::mat4                   _sInverseRotateM;
        glm::mat4                   _sVRInverseRotateM;
        glm::mat4                   _modelM;
        glm::mat4                   _scaleM;
        glm::mat4                   _mvpM;
        glm::mat4                   _mvpTakePhotoM;
        glm::mat4                   _defaultRotateM;
        GLuint                      _textures[3];

		GLuint                      _sProgramPlay;
        GLint                       _aVertexPosHandlePlay;
        GLint                       _aVertexUVHandlePlay;
		GLint                       _uMvpMatrixLocPlay;
        GLuint                      _vboBuffers[2];
        GLuint                      _uCameraTextureLocPlay;

        GLuint                      _sFilterProgramPlay;
        GLint                       _aFilterVertexPosHandlePlay;
        GLint                       _aFilterVertexUVHandlePlay;
        GLint                       _uFilterMvpMatrixLocPlay;
        GLint                       _uFilterCameraTextureLocPlay;

        //logo
        Image                        _logoImage;
        GLint                       _uLogoTextureLocPlay;
        GLint                       _uLogoAngleLocPlay;
        float                       _logoAngle;
        GLint                       _uFilterLogoTextureLocPlay;
        GLint                       _uFilterLogoAngleLocPlay;

        //3dlut
        GLint                       _uFilterLutSizeLocPlay;
        int                         _lutSize;
        GLint                       _uFilterLutTextureLocPlay;
        GLint                       _uLutSizeLocPlay;
        GLint                       _uLutTextureLocPlay;
        unsigned char*             _lutTextureRGB;

        GLint                       _uFilterGpuImageLocPlay;
        GLint                       _uGpuImageLocPlay;
        Filter_Type                 _filterType;

        GLuint                      _pboIdsBuffers[1];
        GLubyte*                    _pboBytes;
        const float                _takePhotoVUV[30] = {
            -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 0.0f
        };

        VideoMediaCodec*             _videoMediaCodec;
        AudioMediaCodec*             _audioMediaCodec;
        ViewMode                     _viewMode;
        RenderMode                   _renderMode;
        OptionMode                   _optionMode;
    private:
        int   _saveFilter;
        int   _animationFrameRestNum;
        float _cameraPosDis;
        float _fovAngleDis;
        float _zNearDis;
        float _zFarDis;
        float _scaleDis;
        float _yRotateAngleDis;
        float _xRotateVelocity;
        float _yRotateVelocity;
        void doAnimation();
        void doAnimationFling();
    public:
        void nativeInitGL();
        void nativeOnSurfaceChanged(float pNewWidth, float pNewHeight);
        void nativeLoadLogoImage(unsigned char* imageData, ssize_t unpackedLen);
        void deInitGL();
        void nativeDrawFrame();
        void nativeDrawFilterFrame();
        void nativeOnResume();
        void nativeOnPause();
        void nativeScreenShot(std::string screenDir);
        GLint nativeGetTextureId();
        void nativeLoadAssets(int fd, off_t outStart, off_t outLen, ANativeWindow* window, int width, int height);
        void nativeLoadSource(const char* filePath, ANativeWindow* window, int width, int height);
        void nativeSetPlaying(bool play);
        void nativeUpdateProgress(float progress);
        void nativeRestart();
        void nativeSetSaveFilter(int saveFilter);
        void updateScale(float scale);
        void updateRotate(float xMove, float yMove);
        void updateRotateFling(float velocityX, float velocityY);
        void updateSensorRotate(glm::mat4 sRotateMat);
        void updateViewMode(ViewMode viewMode);
        void updateRenderMode(RenderMode renderMode);
        void updateOptionMode(OptionMode optionMode);
        void updateLutFilter(AAssetManager* mgr, const char* filePath, int filterType);
        void setOnProgressCallback(jobject onProgress_callback_obj, jmethodID onProgress);
        void updateLogoAngle(float angle);
        void callback(uint8_t* data, ssize_t size);
        virtual void handle(LooperMessage* looperMessage);
};
