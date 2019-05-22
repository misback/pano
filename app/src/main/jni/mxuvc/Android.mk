LOCAL_PATH:= $(call my-dir)

# Default backends
#AUDIO?=libusb-uac
#VIDEO?=libusb-uvc
#AUDIO?=alsa
VIDEO?=v4l2

common_CFLAGS := -Wall -fexceptions \
	-Wcast-align -Wcast-qual -Wextra \
	-Wno-empty-body -Wno-unused-parameter \
	-Wshadow -Wwrite-strings -Wswitch-default

include $(CLEAR_VARS)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/common

FILE_VIDEO := $(wildcard $(LOCAL_PATH)/video/$(VIDEO)/*.c)
FILE_AUDIO := $(wildcard $(LOCAL_PATH)/audio/$(AUDIO)/*.c)
FILE_LIBUSB := $(wildcard $(LOCAL_PATH)/common/libusb/*.c)

LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)

LOCAL_SRC_FILES:= \
	common/common.c \
	common/debug.c \
	$(FILE_VIDEO:$(LOCAL_PATH)/%=%) \
	$(FILE_AUDIO:$(LOCAL_PATH)/%=%)


ifeq ($(VIDEO),libusb-uvc)
LOCAL_SRC_FILES += $(FILE_LIBUSB:$(LOCAL_PATH)/%=%)
else ifeq ($(AUDIO), libusb-uac)
LOCAL_SRC_FILES += $(FILE_LIBUSB:$(LOCAL_PATH)/%=%)
endif

#LOCAL_SDK_VERSION := 9
#LOCAL_NDK_STL_VARIANT := stlport_static

#LOCAL_MODULE_PATH:= $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= libmxuvc
LOCAL_MODULE_TAGS:= optional


LOCAL_CFLAGS += $(common_CFLAGS)
LOCAL_CFLAGS += -ggdb -DLOG_NDEBUG=0

LOCAL_LDLIBS := \
    -lpthread -ldl -lrt

include $(BUILD_SHARED_LIBRARY)

include $(all-subdir-makefiles)
