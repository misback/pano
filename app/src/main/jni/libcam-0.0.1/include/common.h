#include <jni.h>
#include <android/log.h>
#include <libgen.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define STRINGIFY(A)                #A
#define LOG_TAG                     "libcam"

#define	SAFE_FREE(p)				{ if (p) { free((p)); (p) = NULL; } }
#define	SAFE_DELETE(p)				{ if (p) { delete (p); (p) = NULL; } }
#define	SAFE_DELETE_ARRAY(p)		{ if (p) { delete [](p); (p) = NULL; } }
#define	NUM_ARRAY_ELEMENTS(p)		((int) sizeof(p) / sizeof(p[0]))

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

#if NDK_DEBUG == 1
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
#else
#define LOGV(...) do {} while(0)
#define LOGD(...) do {} while(0)
#define LOGI(...) do {} while(0)
#define LOGW(...) do {} while(0)
#define LOGE(...) do {} while(0)
#define LOGF(...) do {} while(0)
#endif