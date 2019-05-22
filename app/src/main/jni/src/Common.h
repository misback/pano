#pragma once

#include <jni.h>
#include <android/log.h>
#include <libgen.h>
#include <unistd.h>
#include <time.h>
#include <string>

#define STRINGIFY(A)                #A
#define LOG_TAG                     "FURY_Jni"

#define	SAFE_FREE(p)				{ if (p) { free((p)); (p) = NULL; } }
#define	SAFE_DELETE(p)				{ if (p) { delete (p); (p) = NULL; } }
#define	SAFE_DELETE_ARRAY(p)		{ if (p) { delete [](p); (p) = NULL; } }
#define	NUM_ARRAY_ELEMENTS(p)		((int) sizeof(p) / sizeof(p[0]))
#define CC_BREAK_IF(cond)           if(cond) break
#if defined(__GNUC__)
// the macro for branch prediction optimaization for gcc(-O2/-O3 required)
#define	CONDITION(cond)				((__builtin_expect((cond)!=0, 0)))
#define	LIKELY(x)					((__builtin_expect(!!(x), 1)))	// x is likely true
#define	UNLIKELY(x)					((__builtin_expect(!!(x), 0)))	// x is likely false
#else
#define	CONDITION(cond)				((cond))
#define	LIKELY(x)					((x))
#define	UNLIKELY(x)					((x))
#endif

#include <assert.h>
#define CHECK(CONDITION) { bool RES = (CONDITION); assert(RES); }
#define CHECK_EQ(X, Y) { bool RES = (X == Y); assert(RES); }
#define CHECK_NE(X, Y) { bool RES = (X != Y); assert(RES); }
#define CHECK_GE(X, Y) { bool RES = (X >= Y); assert(RES); }
#define CHECK_GT(X, Y) { bool RES = (X > Y); assert(RES); }
#define CHECK_LE(X, Y) { bool RES = (X <= Y); assert(RES); }
#define CHECK_LT(X, Y) { bool RES = (X < Y); assert(RES); }

#ifdef LOG_NDEBUG
//#undef LOG_NDEBUG
#endif

//#if NDK_DEBUG == 1
#define LOGV(FMT, ...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "[%d*%s:%d:%s]:" FMT, \
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define LOGD(FMT, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[%d*%s:%d:%s]:" FMT, \
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define LOGI(FMT, ...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[%d*%s:%d:%s]:" FMT, \
						gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define LOGW(FMT, ...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "[%d*%s:%d:%s]:" FMT, \
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define LOGE(FMT, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "[%d*%s:%d:%s]:" FMT, \
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define LOGF(FMT, ...) __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, "[%d*%s:%d:%s]:" FMT, \
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
/*#else
#define LOGV(...) do {} while(0)
#define LOGD(...) do {} while(0)
#define LOGI(...) do {} while(0)
#define LOGW(...) do {} while(0)
#define LOGE(...) do {} while(0)
#define LOGF(...) do {} while(0)
#endif*/


#ifndef	LOG_ASSERT
#if defined(__ANDROID__)
#define	LOG_ASSERT(cond, ...) ( (CONDITION(cond)) \
                            ? ((void)__android_log_assert(#cond, LOG_TAG, ## __VA_ARGS__)) \
                            : (void)0 )
#else
#define	LOG_ASSERT(cond, ...)  do { (void)(CONDITION(cond)); } while(0)
#endif
#endif

#define	ENTER()				LOGD("begin")
#define	RETURN(code,type)	{type RESULT = code; LOGD("end (%d)", (int)RESULT); return RESULT;}
#define	RET(code)			{LOGD("end"); return code;}
#define	EXIT()				{LOGD("end"); return;}
#define	PRE_EXIT()			LOGD("end")

//#define RESERVE_SPACE_SIZE  (1024*2560)
#define RESERVE_SPACE_SIZE  (1024*500)
#ifdef __cplusplus
extern "C" {
#endif
   // size_t getFreeSpace(const char* disk);
    bool  isHasEnoughSpace(const char* disk);
    int64_t systemnanotime();
    int64_t systemmicrotime();
    unsigned long get_file_size(const char *path);
#ifdef __cplusplus
}
#endif
#define GEN_CAMERA_FILENAME(OUT_FILENAME_STR, DIR, FORMAT) \
    struct timeval curTime; \
    gettimeofday(&curTime,0); \
    time_t timep; \
    struct tm *p; \
    time(&timep); \
    p   =   localtime(&timep); \
    long time_use= curTime.tv_sec; \
    char photoFileName[1024]; \
    sprintf(photoFileName, "%s/%04d%02d%02d%02d%02d%02d_%ld.%s", DIR, (1900+p->tm_year), (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, time_use, FORMAT); \
    (OUT_FILENAME_STR) = std::string(photoFileName);

#define GEN_CAMERA_PHOTO_FILENAME(OUT_FILENAME_STR, DIR, FORMAT) \
    struct timeval curTime; \
    gettimeofday(&curTime,0); \
    time_t timep; \
    struct tm *p; \
    time(&timep); \
    p   =   localtime(&timep); \
    long time_use= curTime.tv_sec; \
    char photoFileName[512]; \
    sprintf(photoFileName, "%04d%02d%02d%02d%02d%02d_%ld.%s", (1900+p->tm_year), (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, time_use, FORMAT); \
    char pathPhotoFileName[1024]; \
    sprintf(pathPhotoFileName, "%s/%s", DIR, photoFileName); \
    (OUT_FILENAME_STR) = std::string(pathPhotoFileName);

#define GEN_CAMERA_MP4_FILENAME(OUT_FILENAME_STR, DIR, FORMAT) \
    struct timeval curTime; \
    gettimeofday(&curTime,0); \
    time_t timep; \
    struct tm *p; \
    time(&timep); \
    p   =   localtime(&timep); \
    long time_use= curTime.tv_sec; \
    char mediaFileName[512]; \
    sprintf(mediaFileName, "%04d%02d%02d%02d%02d%02d_%ld", (1900+p->tm_year), (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, time_use); \
    char pathMediaFileName[1024]; \
    sprintf(pathMediaFileName, "%s/%s.%s", DIR, mediaFileName, FORMAT); \
    (OUT_FILENAME_STR) = std::string(pathMediaFileName);

std::string getFileExtension(const std::string& filePath);
#define LONG_TO_OBJ_PTR(OBJ, LONG) ((OBJ*)LONG)
