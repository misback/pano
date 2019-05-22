#pragma once
#include <time.h>
#include <math.h>
#include <android/log.h>
#include <GLES3/gl3.h>
#include "Singleton.h"
#include "Bitmap.h"
#include "android/asset_manager.h"
#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include "glm.hpp"
#include "ext.hpp"
#include <thread>
#include "JniHelper.h"
#include "LutUtil.h"
#include "SphereData.h"
#include "GLUtil.h"
#include "media/NdkMediaCodec.h"
#include "media/NdkMediaFormat.h"
#include <semaphore.h>
#include "Image.h"
#include "TaskMessage.h"
#include "TaskFaceMessage.h"
#include "CameraMediaCodec.h"
class CameraVideoRender:public TaskMessage, public TaskFaceMessage{
public:
    //0:开始拍照,1:没有连接相机,2:正在拍照中,3:空间不够...  tickCount为连拍次数
    enum class TAKE_PHOTO_RESULT{
        START, NOT_CONNECT, PHOTOING, NOT_SPACE, COMPLETE
    };
    //0:相机连接不成,1:相机连接成功
    enum class CODE_CONNECT_CAMERA{
        FAIL                =   0,
        SUCCESS             =   1
    };
    enum class CODE_DISCONNECT_CAMERA{
        FAIL                =   0,
        SUCCESS             =   1
    };
	public:
        CameraVideoRender(const char* tDetectModelPath, const char* tAlignModelPath);
        CameraVideoRender& operator=(const CameraVideoRender& ) = delete;
        CameraVideoRender(CameraVideoRender&) = delete;
        virtual ~CameraVideoRender() ;
    private:
        int                         _widgetWidth;
        int                         _widgetHeight;
        int                         _diagonalLength;
        int                         _takePhotoWidth;
        int                         _takePhotoHeight;
        int                         _faceWidth;
        int                         _faceHeight;
        int                         _faceCalWidth;
        int                         _faceCalHeight;
        int                         _useFaceDetector;
        SphereData*                 _sphereData;
        int                         _recordWidth;
        int                         _recordHeight;
        std::string                 _sPhotoFileName;
        std::string                 _mediaDir;
        std::string                 _effectFilePath;
        AAssetManager*              _aAssetManager;

        jobject                     _renderCallbackObj;
        jmethodID                   _callbackTakePhotoMethod;
    public:
        void                        nativeOnCreate(const char* mediaDir, int shpereVertexNum, float radius, AAssetManager* aAssetManager);
        void                        nativeInitGL( float recordWidth, float recordHeight, int takePhotoWidth, int takePhotoHeight, jobject render_callback_obj, jmethodID callbackTakePhotoMethod, int timeScale, int frameRate);
        void                        nativeOnSurfaceChanged(float pNewWidth, float pNewHeight);
        void                        nativeLoadLogoImage(unsigned char* imageData, ssize_t unpackedLen);
        void                        nativeDrawFrame();
        void                        nativeDrawPanoramaFrame();
        void                        nativeOnResume();
        void                        nativeOnPause();
        void                        nativeCallBackFaceDetector(int width, int height, float x, float y, float eyesDistance);
        void                        deInitGL();
        GLint                        nativeGetTextureId();
        void                        nativeSetSurface(ANativeWindow* aNativeWindow);
        void                        updateLutFilter(AAssetManager* mgr, const char* filePath, int filterType, int useFaceDetector);
        void                        nativeTakePhoto(int tickCount);
        CODE_CONNECT_CAMERA         nativeConnect(int vid, int pid, int fd, const char *usbFs, ANativeWindow* window);
        CODE_DISCONNECT_CAMERA      nativeDisConnect();
        void                        updateScale(float scale);
        void                        updateRotate(float xMove, float yMove);
        void                        updateViewMode(ViewMode viewMode);
        void                        updateLogoAngle(float angle);
        void                        updateEffect(std::string effectFilePath, int useFaceDetector);

        CameraMediaCodec*           _cameraMediaCodec;
    private:
        float                      _scale;
        float                      _fovAngle;
        float                      _faceFovAngle;
        float                      _zFaceNear;
        float                      _zFaceFar;
        float                      _zNear;
        float                      _zFar;
        float                      _cameraPos;
        float                      _cameraFacePos;
        float                      _yRotateAngle;
        float                      _xRotateAngle;
        glm::mat4                   _identityM;
        glm::mat4                   _projectionM;
        glm::mat4                   _projectionFaceM;
        glm::mat4                   _viewM;
        glm::mat4                   _viewFaceM;
        glm::mat4                   _rotateM;
        glm::mat4                   _modelM;
        glm::mat4                   _scaleM;
        glm::mat4                   _mvpM;
        glm::mat4                   _mvpFaceM;
        glm::mat4                   _mvpTakePhotoM;
        glm::mat4                   _faceScaleM;
        glm::mat4                   _faceRotateM;
        bool                       _running;

        GLuint                      _sFilterProgramPlay;
        GLint                       _aFilterVertexPosHandlePlay;
        GLint                       _aFilterVertexUVHandlePlay;
        GLint                       _uFilterMvpMatrixLocPlay;
        GLint                       _uFilterCameraTextureLocPlay;

        GLuint                      _sFaceProgramPlay;
        GLint                       _aFaceVertexPosHandlePlay;
        GLint                       _aFaceVertexUVHandlePlay;
        GLint                       _uFaceMvpMatrixLocPlay;
        GLint                       _uFaceCameraTextureLocPlay;

        GLuint                      _textures[4];

		GLuint                      _sProgramPlay;
        GLint                       _aVertexPosHandlePlay;
        GLint                       _aVertexUVHandlePlay;
		GLint                       _uMvpMatrixLocPlay;
        GLuint                      _vboBuffers[2];
        GLuint                      _uCameraTextureLocPlay;

        //logo
        Image                       _logoImage;
        GLint                       _uLogoTextureLocPlay;
        GLint                       _uLogoAngleLocPlay;
        float                       _logoAngle;
        GLint                       _uFilterLogoTextureLocPlay;
        GLint                       _uFilterLogoAngleLocPlay;
        //effect
        Image                       _effectImage;
        GLint                       _uEffectLatitudeLocPlay;
        GLint                       _uEffectLongitudeLocPlay;
        GLint                       _uEffectFaceSizeLocPlay;

        GLint                       _uFilterEffectLatitudeLocPlay;
        GLint                       _uFilterEffectLongitudeLocPlay;
        GLint                       _uFilterEffectFaceSizeLocPlay;

        float                      _latitude;
        float                      _longitude;
        float                      _faceSize;

        GLint                       _uFilterGpuImageLocPlay;
        GLint                       _uGpuImageLocPlay;
        Filter_Type                 _filterType;

        //3dlut
        GLint                       _uFilterLutSizeLocPlay;
        int                         _lutSize;
        GLint                       _uFilterLutTextureLocPlay;
        unsigned char*             _lutTextureRGB;
        GLuint                      _uLutTextureLocPlay ;
        GLuint                      _uLutSizeLocPlay;

        GLuint                       _frameBuffers[2];
        ViewMode                     _viewMode;
        RenderMode                   _renderMode;

        GLuint                      _pboIdsBuffers[2];
        GLubyte*                    _pboBytes;
        GLubyte*                    _pboFaceBytes;

        const float                _takePhotoVUV[30] = {
            -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 0.0f
        };

        std::atomic<int>           _takePhotoTickCount;
	public:
	    void                       drawRecordFilterFrame();
	    void                       drawFaceFrame();
        void                       drawFilterFrame();
        void                       drawPanoramaFrame();
        void                       setBaseFilter(int baseFilter);
        int                        getBaseFilter();
        bool                       getRaySphereIntersect(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 sphereCenter, float sphereRadius, std::vector<glm::vec3> &intersectPoint);
    private:
        Image                       _image;

    public:
        void updateRotateFling(float velocityX, float velocityY);
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

        virtual void handle(LooperMessage* looperMessage);
        virtual void handleFace(FaceMessage* faceMessage);
};