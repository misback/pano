######################################################################
# libcam_static.a (static library with static link to libjpeg, libusb1.0)
######################################################################
LOCAL_PATH := $(call my-dir)/../..
include $(CLEAR_VARS)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/ \
	$(LOCAL_PATH)/include

LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_CFLAGS += -DANDROID_NDK
LOCAL_CFLAGS += -DLOG_NDEBUG

LOCAL_EXPORT_LDLIBS := -llog

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES += libusb1.0

LOCAL_SRC_FILES := \
	src/cJSON.c \
	src/libmxcam.c \
	src/mxcam.c \

LOCAL_MODULE := libcam001
include $(BUILD_STATIC_LIBRARY)

######################################################################
# libcam.so (shared library with static link to libjpeg-turbo)
######################################################################
