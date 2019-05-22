LOCAL_PATH := $(call my-dir)/../..
include $(CLEAR_VARS)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/common \
	$(LOCAL_PATH)/src/common/libusb \
	$(LOCAL_PATH)/src/common/libskypeecxu \
	$(LOCAL_PATH)/src/video/libusb-uvc

LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_CFLAGS += -DANDROID_NDK
LOCAL_CFLAGS += -DLOG_NDEBUG

LOCAL_EXPORT_LDLIBS := -llog

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES += libusb1.0

LOCAL_SRC_FILES := \
	src/common/common.c \
	src/common/debug.c \
    src/common/qbox.c \
    src/common/yuvutil.c \
    src/common/libusb/handle_events.c \
	src/common/libskypeecxu/skypeecxuparser.c \
	src/video/libusb-uvc/desc-parser.c \
	src/video/libusb-uvc/uvc.c \
	src/plugins/overlay/overlay.c

LOCAL_MODULE := libmxuvc

include $(BUILD_STATIC_LIBRARY)
