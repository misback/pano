######################################################################
# libpng1628.a
######################################################################
LOCAL_PATH := $(call my-dir)/../../..
include $(CLEAR_VARS)

CFLAGS := -Werror

LOCAL_MODULE := png1628

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/

LOCAL_EXPORT_C_INCLUDES := \
		$(LOCAL_PATH)/

LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_CFLAGS += -DANDROID_NDK

LOCAL_CFLAGS += -DAVOID_TABLES
LOCAL_CFLAGS += -O3 -fstrict-aliasing -DPNG_INTEL_SSE
LOCAL_CFLAGS += -fprefetch-loop-arrays


LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	png.c pngerror.c pngget.c \
	pngmem.c pngpread.c pngread.c pngrio.c pngrtran.c pngrutil.c \
	pngset.c pngtrans.c pngwio.c pngwrite.c pngwtran.c pngwutil.c \

include $(BUILD_STATIC_LIBRARY)


