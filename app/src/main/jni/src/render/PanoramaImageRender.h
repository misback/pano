//
// Created by DELL on 2017/3/11.
//
#pragma once
#include "glm.hpp"
#include "ext.hpp"
#include <GLES3/gl3.h>
#include "Common.h"
#include "Singleton.h"
#include "SphereData.h"
#include "GLUtil.h"
#include <atomic>
#include "android/asset_manager.h"
#include <android/asset_manager_jni.h>
#include "Image.h"
#include "TaskMessage.h"
class PanoramaImageRender:public TaskMessage{
    public:
        PanoramaImageRender(const char* mediaDir, jobject panorama_callback_obj, jmethodID cbScreenShotMethod);
        PanoramaImageRender& operator=(const PanoramaImageRender& ) = delete;
        PanoramaImageRender(PanoramaImageRender&) = delete;
        virtual ~PanoramaImageRender();
    private:
        int                         _widgetWidth;
        int                         _widgetHeight;
        int                         _diagonalLength;
        jobject                     _panoramaIamgeCallbackObj;
        jmethodID                   _callbackScreenShotMethod;
        std::string                 _mediaDir;
        std::string                 _screenDir;
        int                         _screenTag;
        SphereData*                 _sphereData;
        float                      _scale;
        float                      _fovAngle;
        float                      _zNear;
        float                      _zFar;
        float                      _cameraPos;
        float                      _xRotateAngle;
        float                      _yRotateAngle;
        float                      _vrRotateAngle;
        glm::mat4                   _identityM;
        glm::mat4                   _projectionM;
        glm::mat4                   _viewM;
        glm::mat4                   _rotateM;
        glm::mat4                   _sRotateM;
        glm::mat4                   _sInverseRotateM;
        glm::mat4                   _sVRInverseRotateM;
        glm::mat4                   _sVrRotateM;
        glm::mat4                   _modelM;
        glm::mat4                   _scaleM;
        glm::mat4                   _mvpM;
        glm::mat4                   _mvpTakePhotoM;

        GLuint                      _textures[4];

		GLuint                      _sProgramPlay;
        GLint                       _aVertexPosHandlePlay;
        GLint                       _aVertexUVHandlePlay;
		GLint                       _uMvpMatrixLocPlay;
        GLuint                      _vboBuffers[2];
        GLuint                      _uPanoramaImageTextureLocPlay ;

        GLuint                      _sFilterProgramPlay;
        GLint                       _aFilterVertexPosHandlePlay;
        GLint                       _aFilterVertexUVHandlePlay;
        GLint                       _uFilterMvpMatrixLocPlay;
        GLuint                      _uFilterPanoramaImageTextureLocPlay ;

        //logo
        Image                        _logoImage;
        GLint                       _uLogoTextureLocPlay;
        GLint                       _uLogoAngleLocPlay;
        float                       _logoAngle;
        GLint                       _uFilterLogoTextureLocPlay;
        GLint                       _uFilterLogoAngleLocPlay;

        //3dlut
        GLint                       _uLutSizeLocPlay;
        int                         _lutSize;
        GLint                       _uLutTextureLocPlay;
        unsigned char*             _lutTextureRGB;
        GLint                       _uFilterLutSizeLocPlay;
        GLint                       _uFilterLutTextureLocPlay;

        GLint                       _uFilterGpuImageLocPlay;
        GLint                       _uGpuImageLocPlay;
        Filter_Type                 _filterType;

        GLuint                      _frameBuffers[1];
        GLuint                      _pboIdsBuffers[2];
        GLubyte*                    _pboSaveBytes;
        GLubyte*                    _pboBytes;
        const float                _takePhotoVUV[30] = {
            -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 0.0f
        };

        Image                        _image;
        ViewMode                     _viewMode;
        RenderMode                   _renderMode;
        OptionMode                   _optionMode;

    private:
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
        void nativeLoadImage(unsigned char* imageData, ssize_t unpackedLen);
        void nativeLoadLogoImage(unsigned char* imageData, ssize_t unpackedLen);
        void deInitGL();
        void nativeSaveFilterFrame(const char* dir);
        void nativeDrawFrame();
        void nativeOnResume();
        void nativeOnPause();
        void nativeScreenShot(std::string screenDir);
        void updateScale(float scale);
        void updateRotate(float xMove, float yMove, int notAnimation);
        void updateRotateFling(float velocityX, float velocityY);
        void updateSensorRotate(glm::mat4 sRotateMat);
        void updateViewMode(ViewMode viewMode);
        void updateRenderMode(RenderMode renderMode);
        void updateOptionMode(OptionMode optionMode);
        void updateLutFilter(AAssetManager* mgr, const char* filePath, int filterType);
        void updateLogoAngle(float angle);

        virtual void handle(LooperMessage* looperMessage);
};
