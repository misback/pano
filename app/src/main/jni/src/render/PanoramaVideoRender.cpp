//
// Created by DELL on 2017/3/11.
//
#include "PanoramaVideoRender.h"
#include "ImageUtil.h"
#include "math.h"
#include "shader/panorama_video_position.vert"
#include "shader/panorama_video_play.frag"
#include "shader/filter_video_position.vert"
#include "shader/filter_video_play.frag"
#include "LutUtil.h"
#include "JniHelper.h"
PanoramaVideoRender::PanoramaVideoRender(const char* mediaDir, jobject panorama_callback_obj, jmethodID cbScreenShotMethod, jmethodID callbackSaveFilterMethod):
_sProgramPlay(0),
_aVertexPosHandlePlay(0),
_aVertexUVHandlePlay(0),
_uMvpMatrixLocPlay(0),
_lutSize(0),
_lutTextureRGB(nullptr),
_uLogoAngleLocPlay(0),
_logoAngle(15.0),
_renderMode(RenderMode::SINGLE),
_optionMode(OptionMode::FINGER),
_scale(SCALE_DEFAULT_FISH),
_xRotateAngle(0.0),
_yRotateAngle(0.0),
_vrRotateAngle(0.0),
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
_videoWidth(0),
_videoHeight(0),
_screenTag(0),
_panoramaVideoCallbackObj(panorama_callback_obj),
_callbackScreenShotMethod(cbScreenShotMethod),
_callbackSaveFilterMethod(callbackSaveFilterMethod),
_pboBytes(nullptr),
_filterType(Filter_Type::NONE),
_mediaDir(mediaDir),
_saveFilter(0)
{
    _videoMediaCodec  =   new VideoMediaCodec();
    _audioMediaCodec  =   new AudioMediaCodec(this);
    _identityM      =   glm::mat4(1.0f);
    _modelM         =   glm::mat4(1.0f);
    _scaleM         =   glm::mat4(1.0f);
    _rotateM        =   glm::mat4(1.0f);
    _sRotateM       =   glm::mat4(1.0f);
    _sInverseRotateM=   glm::mat4(1.0f);
    /*float rotateVRMatrix[16] = {
        0.07840727f,    -0.7055347f,    0.7042986f, 0.0f,
        0.054867953f,   0.7084749f,     0.7035917f, 0.0f,
        -0.9953921f,    -0.016509652f,  0.09426689f,0.0f,
        0.0f,           0.0f,           0.0f,       1.0f
    };*/
    float rotateVRMatrix[16] = {
        0.058295447f,    0.27763367f,    -0.9589056f, 0.0f,
        -0.019285336f,   0.96068364f,    0.276973f,   0.0f,
        0.9981024f,      0.0023497194f,  0.061358213f,0.0f,
        0.0f,            0.0f,           0.0f,        1.0f
    };
    glm::quat quaternion = glm::toQuat(glm::make_mat4(rotateVRMatrix));
    glm::vec3 euler = glm::eulerAngles(quaternion);
    _sVRInverseRotateM = glm::inverse(glm::orientate4(glm::vec3(0.0f, euler.x+glm::radians(180.f), euler.z)));
    //_sVRInverseRotateM = glm::inverse(glm::make_mat4(rotateVRMatrix));
    _sVrRotateM     =   glm::rotate(glm::radians(-90.f), glm::vec3(0, 0, 1));
    _mvpTakePhotoM  =   glm::mat4(1.0f);
    _sphereData     =   new SphereData();
    _sphereData->init(100, 1.0f);
    updateViewMode(ViewMode::FISH);
}
PanoramaVideoRender::~PanoramaVideoRender(){
    _panoramaVideoCallbackObj = nullptr;
    deInitGL();
    if(_lutTextureRGB != nullptr){
        delete[] _lutTextureRGB;
        _lutTextureRGB  =   nullptr;
    }
    if(_sphereData != nullptr){
        delete _sphereData;
        _sphereData = nullptr;
    }
    if(_videoMediaCodec != nullptr){
        delete _videoMediaCodec;
        _videoMediaCodec = nullptr;
    }
    if(_audioMediaCodec != nullptr){
        delete _audioMediaCodec;
        _audioMediaCodec = nullptr;
    }
}
void PanoramaVideoRender::deInitGL(){
    glDeleteTextures(3, _textures);
    glDeleteBuffers(2, _vboBuffers);
    glDeleteBuffers(1, _pboIdsBuffers );
    if(_sProgramPlay){
     	glDeleteProgram(_sProgramPlay);
    }
    _lutSize    =   0;
}
void PanoramaVideoRender::nativeInitGL(){
    _diagonalLength         =   sqrt(_widgetWidth*_widgetWidth + _widgetHeight*_widgetHeight);
    if(CompileShaderProgram(panorama_video_position_vert, panorama_video_play_frag, &_sProgramPlay) && CompileShaderProgram(filter_video_position_vert, filter_video_play_frag, &_sFilterProgramPlay)){
        _aVertexPosHandlePlay                       = glGetAttribLocation(_sProgramPlay,           "a_VertexPos");
        _aVertexUVHandlePlay                        = glGetAttribLocation(_sProgramPlay,           "a_VertexUV");
        _uMvpMatrixLocPlay                          = glGetUniformLocation(_sProgramPlay,          "u_MvpMatrix");
        _uCameraTextureLocPlay                      = glGetUniformLocation(_sProgramPlay,          "u_CameraTextureOES");
        _uLutTextureLocPlay                         = glGetUniformLocation(_sProgramPlay,          "u_LutTexture");
        _uLutSizeLocPlay                            = glGetUniformLocation(_sProgramPlay,          "u_LutSize");
        _uLogoTextureLocPlay                        = glGetUniformLocation(_sProgramPlay,          "u_LogoTexture");
        _uLogoAngleLocPlay                          = glGetUniformLocation(_sProgramPlay,          "u_LogoAngle");
        _uGpuImageLocPlay                           = glGetUniformLocation(_sProgramPlay,          "u_GpuImage");

        _aFilterVertexPosHandlePlay                 = glGetAttribLocation(_sFilterProgramPlay,           "a_VertexPos");
        _aFilterVertexUVHandlePlay                  = glGetAttribLocation(_sFilterProgramPlay,           "a_VertexUV");
        _uFilterMvpMatrixLocPlay                    = glGetUniformLocation(_sFilterProgramPlay,          "u_MvpMatrix");
        _uFilterCameraTextureLocPlay                = glGetUniformLocation(_sFilterProgramPlay,          "u_CameraTextureOES");
        _uFilterLutTextureLocPlay                   = glGetUniformLocation(_sFilterProgramPlay,          "u_LutTexture");
        _uFilterLutSizeLocPlay                      = glGetUniformLocation(_sFilterProgramPlay,          "u_LutSize");
        _uFilterLogoTextureLocPlay                  = glGetUniformLocation(_sFilterProgramPlay,          "u_LogoTexture");
        _uFilterLogoAngleLocPlay                    = glGetUniformLocation(_sFilterProgramPlay,          "u_LogoAngle");
        _uFilterGpuImageLocPlay                     = glGetUniformLocation(_sFilterProgramPlay,          "u_GpuImage");

        glGenBuffers(2, _vboBuffers);
        glBindBuffer(GL_ARRAY_BUFFER, _vboBuffers[0]);
        glBufferData(GL_ARRAY_BUFFER, _sphereData->_size*20, _sphereData->_vertexUVBuff, GL_STATIC_DRAW );
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, _vboBuffers[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(_takePhotoVUV), _takePhotoVUV, GL_STATIC_DRAW );
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenTextures(3, _textures);
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

        glGenBuffers(1, _pboIdsBuffers);
    }else{
        LOGE("CompileShaderProgram");
    }
}
void PanoramaVideoRender::nativeOnSurfaceChanged(float pNewWidth, float pNewHeight){
    _widgetWidth            =   pNewWidth;
    _widgetHeight           =   pNewHeight;
    _diagonalLength         =   sqrt(_widgetWidth*_widgetWidth + _widgetHeight*_widgetHeight);
}
void PanoramaVideoRender::nativeDrawFrame(){
    doAnimation();
    glClear(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    switch(_renderMode){
        case RenderMode::SINGLE:{
            _viewM = glm::lookAt(
                glm::vec3(0,0,_cameraPos),
                glm::vec3(0,0,-1),
                glm::vec3(0,1,0)
            );
            _projectionM = glm::perspective(glm::radians(_fovAngle),(float)_widgetWidth/(float)_widgetHeight*(_renderMode == RenderMode::SINGLE ? 1.0f:0.5f),_zNear, _zFar);
            _scaleM = glm::scale(glm::mat4(1.0f), glm::vec3(_scale));
            _mvpM = _projectionM * _viewM * _sRotateM * _sInverseRotateM * _rotateM * _scaleM;
        }
            break;
        case RenderMode::DOUBLE:{
            _viewM = glm::lookAt(
                glm::vec3(0,0,_cameraPos),
                glm::vec3(0,0,-1),
                glm::vec3(1,0,0)
            );
            _projectionM = glm::perspective(glm::radians(_fovAngle),(float)_widgetWidth/(float)_widgetHeight*(_renderMode == RenderMode::SINGLE ? 1.0f:0.5f),_zNear, _zFar);
            _scaleM = glm::scale(glm::mat4(1.0f), glm::vec3(_scale));
            _mvpM = _projectionM * _viewM * _sRotateM * _sVRInverseRotateM * _rotateM * _sVrRotateM * _scaleM;
        }
            break;
    }
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
    glUniform1f(_uLutSizeLocPlay, (float)_lutSize);
    if (_lutSize > 0 ){
        glUniform1i(_uLutTextureLocPlay, 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _textures[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _lutSize*_lutSize, _lutSize, 0, GL_RGB, GL_UNSIGNED_BYTE, _lutTextureRGB);
    }else{
        glUniform1i(_uLutTextureLocPlay, 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _textures[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _lutSize*_lutSize, _lutSize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
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
    switch(_renderMode){
        case RenderMode::SINGLE:{
            glViewport(0,0,_widgetWidth, _widgetHeight);
            glDrawArrays(GL_TRIANGLES, 0, _sphereData->_size);
        }
            break;
        case RenderMode::DOUBLE:{
            glViewport(0, 0, _widgetWidth/2.f, _widgetHeight);
            glDrawArrays(GL_TRIANGLES, 0, _sphereData->_size);
            glViewport(_widgetWidth/2.f, 0, _widgetWidth/2.f, _widgetHeight);
            glDrawArrays(GL_TRIANGLES, 0, _sphereData->_size);
        }
            break;
    }
    glDisableVertexAttribArray(_aVertexPosHandlePlay);
    glDisableVertexAttribArray(_aVertexUVHandlePlay);
    if(_screenTag == 1 && _pboBytes ==  nullptr){
        _screenTag = 0;
        glBindBuffer(GL_PIXEL_PACK_BUFFER, _pboIdsBuffers[0]);
        glBufferData(GL_PIXEL_PACK_BUFFER, _widgetWidth*_widgetHeight*4, nullptr, GL_DYNAMIC_READ);
        glReadPixels(0, 0, _widgetWidth, _widgetHeight, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, _pboIdsBuffers[0]);
        _pboBytes = (GLubyte*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, _widgetWidth*_widgetHeight*4, GL_MAP_READ_BIT);
        if (_pboBytes!=nullptr){
            WorkerData* _workerData = new WorkerData(_pboBytes, _widgetWidth, _widgetHeight, _widgetWidth*_widgetHeight*4);
            post(TaskMessageType::kMsgSaveData, _workerData);
        }
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }
    CheckOpenGL("mmmmfff1");
}
void PanoramaVideoRender::nativeDrawFilterFrame(){
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, _videoWidth, _videoHeight);
    glUseProgram(_sFilterProgramPlay);
    glUniformMatrix4fv(_uFilterMvpMatrixLocPlay, 1, false, glm::value_ptr(_mvpTakePhotoM));
    glUniform1i(_uFilterCameraTextureLocPlay, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, _textures[0]);
    //filter
    glUniform1f(_uFilterLutSizeLocPlay, (float)_lutSize);
    if(_lutSize > 0.f && _lutTextureRGB != nullptr){
        glUniform1i(_uFilterLutTextureLocPlay, 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _textures[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _lutSize*_lutSize, _lutSize, 0, GL_RGB, GL_UNSIGNED_BYTE, _lutTextureRGB);
    }else{
        glUniform1i(_uFilterLutTextureLocPlay, 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _textures[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _lutSize*_lutSize, _lutSize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
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
GLint PanoramaVideoRender::nativeGetTextureId(){
    return _textures[0];
}
void PanoramaVideoRender::nativeOnResume(){
}
void PanoramaVideoRender::nativeOnPause(){
}
void PanoramaVideoRender::nativeScreenShot(std::string screenDir){
    if(_screenTag != 1){
        _screenDir = screenDir;
        _screenTag = 1;
    }
}
void PanoramaVideoRender::nativeLoadSource(const char* filePath, ANativeWindow* window, int width, int height){
    _videoWidth = width;
    _videoHeight = height;
    LOGE("nativeLoadSource: %s", filePath);
    {
        auto aMediaExtractor = AMediaExtractor_new();
        auto err = AMediaExtractor_setDataSource(aMediaExtractor, filePath);
        if (err == AMEDIA_OK) {
            _videoMediaCodec->loadSource(aMediaExtractor, window);
        }else{
            LOGE("setDataSource error: %d", err);
        }
    }
    {
        auto aMediaExtractor = AMediaExtractor_new();
        auto err = AMediaExtractor_setDataSource(aMediaExtractor, filePath);
        if (err == AMEDIA_OK) {
            _audioMediaCodec->loadSource(aMediaExtractor);
        }else{
            LOGE("setDataSource error: %d", err);
        }
    }
}
void PanoramaVideoRender::nativeLoadAssets(int fd, off_t outStart, off_t outLen, ANativeWindow* window, int width, int height){
    _videoWidth = width;
    _videoHeight = height;
    // obtain a native window from a Java surface
    {
        auto aMediaExtractor = AMediaExtractor_new();
        media_status_t err = AMediaExtractor_setDataSourceFd(aMediaExtractor, fd, static_cast<off64_t>(outStart), static_cast<off64_t>(outLen));
        close(fd);
        if (err == AMEDIA_OK) {
            _videoMediaCodec->loadSource(aMediaExtractor, window);
        }else{
            LOGE("setDataSource error: %d", err);
        }
    }
    {
        auto aMediaExtractor = AMediaExtractor_new();
        media_status_t err = AMediaExtractor_setDataSourceFd(aMediaExtractor, fd, static_cast<off64_t>(outStart), static_cast<off64_t>(outLen));
        close(fd);
        if (err == AMEDIA_OK) {
            _audioMediaCodec->loadSource(aMediaExtractor);
        }else{
            LOGE("setDataSource error: %d", err);
        }
    }
}
void PanoramaVideoRender::nativeSetPlaying(bool isPlaying){
    _videoMediaCodec->nativeSetPlaying(isPlaying);
    _audioMediaCodec->nativeSetPlaying(isPlaying);
}
void PanoramaVideoRender::nativeUpdateProgress(float progress){
    _videoMediaCodec->nativeUpdateProgress(progress);
    _audioMediaCodec->nativeUpdateProgress(progress);
}
void PanoramaVideoRender::nativeRestart(){
    _videoMediaCodec->nativeRestart();
    _audioMediaCodec->nativeRestart();
}
void PanoramaVideoRender::nativeSetSaveFilter(int saveFilter){
    _saveFilter = saveFilter;
}
void PanoramaVideoRender::updateLogoAngle(float angle){
    _logoAngle = angle;
}
void PanoramaVideoRender::updateScale(float scale){
    if(_animationFrameRestNum > 0){
        return;
    }
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
void PanoramaVideoRender::updateRotate(float xMove, float yMove){
    if(_animationFrameRestNum > 0){
        return;
    }
   if(_optionMode == OptionMode::FINGER){
        _yRotateAngle   +=  yMove;
        if(_yRotateAngle>= Y_ROTATE_ANGLE){
            _yRotateAngle   =   Y_ROTATE_ANGLE;
        }else if(_yRotateAngle<= -Y_ROTATE_ANGLE){
            _yRotateAngle   =   -Y_ROTATE_ANGLE;
        }
    }
    switch(_renderMode){
        case RenderMode::SINGLE:{
            _xRotateAngle   +=  xMove;
            _rotateM        =   glm::rotate(glm::radians(_yRotateAngle), glm::vec3(1, 0, 0))*glm::rotate(glm::radians(_xRotateAngle), glm::vec3(0, 1, 0));
        }
            break;
        case RenderMode::DOUBLE:{
            _vrRotateAngle  +=  xMove;
            _rotateM        =   glm::rotate(glm::radians(_yRotateAngle+_vrRotateAngle), glm::vec3(1, 0, 0))*glm::rotate(glm::radians(_xRotateAngle), glm::vec3(0, 1, 0));
        }
            break;
    }
}
void PanoramaVideoRender::doAnimationFling(){
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
void PanoramaVideoRender::updateRotateFling(float velocityX, float velocityY){
    _xRotateVelocity    =   velocityX;
    _yRotateVelocity    =   velocityY;
}
void PanoramaVideoRender::updateViewMode(ViewMode viewMode){
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
void PanoramaVideoRender::doAnimation(){
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
void PanoramaVideoRender::updateRenderMode(RenderMode renderMode){
    _renderMode = renderMode;
    _sRotateM = glm::mat4(1.0f);
    _sInverseRotateM = glm::mat4(1.0f);
    _xRotateAngle = 0.0f;
    _yRotateAngle = 0.0f;
    _vrRotateAngle = 0.0f;
    _rotateM        =   glm::mat4(1.0f);
}
void PanoramaVideoRender::updateOptionMode(OptionMode optionMode){
    _optionMode = optionMode;
    if(_optionMode == OptionMode::FINGER){
        _sRotateM = glm::mat4(1.0f);
        _sInverseRotateM = glm::mat4(1.0f);
        _xRotateAngle = 0.0f;
        _yRotateAngle = 0.0f;
        _vrRotateAngle = 0.0f;
    }
    updateViewMode(_viewMode);
}
void PanoramaVideoRender::updateSensorRotate(glm::mat4 sRotateMat){
    _sRotateM = sRotateMat;
    switch(_renderMode){
        case RenderMode::SINGLE:{
            if(_sInverseRotateM == glm::mat4(1.0f)){
                _sInverseRotateM = glm::inverse(_sRotateM);
            }
        }
            break;
        case RenderMode::DOUBLE:{
        }
            break;
    }
}
void PanoramaVideoRender::setOnProgressCallback(jobject onProgress_callback_obj, jmethodID onProgress){
    _videoMediaCodec->setOnProgressCallback(onProgress_callback_obj, onProgress);
}
void PanoramaVideoRender::updateLutFilter(AAssetManager* mgr, const char* filePath, int filterType){
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
void PanoramaVideoRender::nativeLoadLogoImage(unsigned char* imageData, ssize_t unpackedLen){
    if(imageData == nullptr){
        _logoImage.clear();
    }else{
        _logoImage.initWithImageData(imageData, unpackedLen);
    }
}
void PanoramaVideoRender::callback(uint8_t* data, ssize_t size){
    if(_saveFilter == 1 && _panoramaVideoCallbackObj != nullptr){
        JNIEnv *env =   JniHelper::getEnv();
        jobject buf = env->NewDirectByteBuffer(data, size);
        env->CallVoidMethod(_panoramaVideoCallbackObj, _callbackSaveFilterMethod, buf, size);
        env->ExceptionClear();
        env->DeleteLocalRef(buf);
    }
}
void PanoramaVideoRender::handle(LooperMessage* looperMessage) {
    auto what = looperMessage->what;
    auto obj = looperMessage->obj;
    switch (what) {
        case TaskMessageType::kMsgSaveData:{
            if(obj->_data == nullptr){
                return;
            }
            std::string _sScreenFileName;
            GEN_CAMERA_PHOTO_FILENAME(_sScreenFileName, _screenDir.c_str(), "jpeg");
            if(_image.saveImageToJPG_FlipY(obj->_data, true, obj->_width, obj->_height, _sScreenFileName.c_str())){
                if (_panoramaVideoCallbackObj != nullptr) {
                    JNIEnv *env =   JniHelper::getEnv();
                    jstring _pathName = env->NewStringUTF(_sScreenFileName.c_str());
                    env->CallVoidMethod(_panoramaVideoCallbackObj, _callbackScreenShotMethod, _pathName, 1);
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