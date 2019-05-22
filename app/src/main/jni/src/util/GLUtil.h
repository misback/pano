#pragma once
#include <jni.h>
#include <errno.h>
#include <stdio.h>
#include <android/log.h>
#include "Common.h"
#include <GLES3/gl3.h>
#define GL_BGRA                       0x80E1
/* GL_OES_EGL_image_external */
#ifndef GL_OES_EGL_image_external
#define GL_TEXTURE_EXTERNAL_OES                                 0x8D65
#define GL_SAMPLER_EXTERNAL_OES                                 0x8D66
#define GL_TEXTURE_BINDING_EXTERNAL_OES                         0x8D67
#define GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES                     0x8D68
#endif

#define DEFAULT_PREVIEW_WIDTH                                   1920
#define DEFAULT_PREVIEW_HEIGHT                                  960
#define DEFAULT_MJPEG_WIDTH                                     2560
#define DEFAULT_MJPEG_HEIGHT                                    1280
#define DEFAULT_PREVIEW_BITRATE                                 8000000
#define DEFAULT_PREVIEW_FPS_MIN                                 1
#define DEFAULT_PREVIEW_FPS_MAX                                 25
#define TAKE_PHOTO_INTERVAL_FRAME                               10

#define CAMERA_POS_DEFAULT_FISH                                 2.0f
#define CAMERA_POS_DEFAULT_PERSPECTIVE                          0.0f
#define CAMERA_POS_DEFAULT_PLANET                               1.0f
#define CAMERA_POS_DEFAULT_CRYSTAL_BALL                         CAMERA_POS_DEFAULT_FISH

#define FOV_DEFAULT_FISH                                        100.f
#define FOV_MAX_DEFAULT_PERSPECTIVE                             140.f
#define FOV_DEFAULT_PERSPECTIVE                                 95.f
#define FOV_MIN_DEFAULT_PERSPECTIVE                             80.f
#define FOV_MAX_PLANET                                          144.f
#define FOV_MIN_PLANET                                          92.f
#define FOV_DEFAULT_PLANET                                      FOV_MAX_PLANET
#define FOV_DEFAULT_CRYSTAL_BALL                                FOV_DEFAULT_FISH

#define Z_NEAR_DEFAULT_FISH                                     1.5f
#define Z_NEAR_DEFAULT_PERSPECTIVE                              0.5f
#define Z_NEAR_DEFAULT_PLANET                                   0.1f
#define Z_NEAR_DEFAULT_CRYSTAL_BALL                             Z_NEAR_DEFAULT_FISH

#define SCALE_MAX_FISH                                          2.5f
#define SCALE_DEFAULT_FISH                                      SCALE_MAX_FISH
#define SCALE_MIN_FISH                                          1.0f
#define SCALE_DEFAULT_PERSPECTIVE                               SCALE_MAX_FISH
#define SCALE_MAX_PLANET                                    SCALE_MAX_FISH
#define SCALE_DEFAULT_PLANET                                    SCALE_MIN_FISH
#define SCALE_MIN_PLANET                                        SCALE_MIN_FISH
#define SCALE_MAX_CRYSTAL_BALL                                  SCALE_MAX_FISH
#define SCALE_DEFAULT_CRYSTAL_BALL                              SCALE_MIN_FISH
#define SCALE_MIN_CRYSTAL_BALL                                  SCALE_MIN_FISH

#define ROTATE_ANGLE_Y_DEFAULT_FISH                             0
#define ROTATE_ANGLE_Y_DEFAULT_PERSPECTIVE                      0
#define ROTATE_ANGLE_Y_DEFAULT_PLANET                           90.f
#define ROTATE_ANGLE_Y_DEFAULT_CRYSTAL_BALL                     0

#define Z_FAR                                                   600.f
#define Y_ROTATE_ANGLE                                          90.0f
#define ANIMATION_FRAME_TIME                                    0.04f
#define ANIMATION_FRAME_NUM                                     25
#define ACCELERATION                                            0.25


enum class ViewMode{
    FISH,
    PERSPECTIVE,
    PLANET,
    CRYSTAL_BALL
};
enum class RenderMode{
    SINGLE,
    DOUBLE
};
enum class OptionMode{
    FINGER,
    GYROSCOPE
};
enum Filter_Type{
    NONE,
    GRAY_RELIEF
};
#ifdef __cplusplus
extern "C" {
#endif
    extern void CheckOpenGL(char* location);
    extern void CheckFrameBufferStatus();
    extern void CheckGlError(const char* op);
    extern void CheckLocation(int location, char* label);
    extern bool CompileShaderFromString( const char* strShaderSource, GLint hShaderHandle );
    extern bool LinkShaderProgram( unsigned int hShaderProgram );
    extern bool CompileShaderProgram( const char* strVertexShader, const char* strFragmentShader, GLuint* pShaderProgramHandle);
#ifdef __cplusplus
}
#endif
