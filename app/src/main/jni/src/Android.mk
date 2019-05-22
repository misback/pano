LOCAL_PATH	:= $(call my-dir)
include $(CLEAR_VARS)

CFLAGS := -Werror

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/ \
		$(LOCAL_PATH)/../ \
		$(LOCAL_PATH)/base/ \
		$(LOCAL_PATH)/decode/ \
		$(LOCAL_PATH)/model/ \
		$(LOCAL_PATH)/render/ \
		$(LOCAL_PATH)/util/ \
		$(LOCAL_PATH)/../glm \
		$(LOCAL_PATH)/../glm/glm \
		$(LOCAL_PATH)/../glm/glm/detail \
		$(LOCAL_PATH)/../glm/glm/gtc \
		$(LOCAL_PATH)/../glm/glm/gtx \
		$(LOCAL_PATH)/../mxuvc/include \
        $(LOCAL_PATH)/../mxuvc/src/common \
        $(LOCAL_PATH)/../mxuvc/src/common/libusb \
        $(LOCAL_PATH)/../mxuvc/src/libusb-uac \
        $(LOCAL_PATH)/../mxuvc/src/common/libskypeecxu \
        $(LOCAL_PATH)/../lpng1628 \
        $(LOCAL_PATH)/../seeta \


LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_CFLAGS += -DHAVE_CONFIG_H
LOCAL_CFLAGS += -fprefetch-loop-arrays -Wno-conversion-null
LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays -O3 -ffast-math -Wno-conversion-null -Wno-write-strings -Wno-error=format-security


LOCAL_LDLIBS :=-lOpenSLES -lGLESv3 -llog -ldl -landroid -latomic -lz -lmediandk


LOCAL_ARM_MODE := arm
LOCAL_STATIC_LIBRARIES += libmxuvc
LOCAL_STATIC_LIBRARIES += png1628
LOCAL_STATIC_LIBRARIES += libjpeg_static
LOCAL_STATIC_LIBRARIES += SeetafaceSo

LOCAL_SRC_FILES := \
		../glm/glm/detail/dummy.cpp \
		../glm/glm/detail/glm.cpp \
        base/Bitmap.cpp \
        base/Image.cpp \
        decode/MediaCodec.cpp \
        decode/VideoMediaCodec.cpp \
        decode/AudioMediaCodec.cpp \
        decode/CameraMediaCodec.cpp \
        model/SphereData.cpp \
        face/FaceDetectManager.cpp \
		render/PanoramaVideoRender.cpp \
		render/PanoramaCameraRender.cpp \
		render/PanoramaImageRender.cpp \
		render/CameraVideoRender.cpp \
		render/TaskFaceMessage.cpp \
		render/TaskMessage.cpp \
		util/ImageUtil.cpp \
		util/GLUtil.cpp \
		util/JniHelper.cpp \
		util/LutUtil.cpp \
        Java_com_uni_vr_PanoramaImageRender.cpp \
        Java_com_uni_vr_PanoramaVideoRender.cpp \
        Java_com_uni_vr_CameraVideoRender.cpp \
        Java_com_uni_vr_PanoramaCameraRender.cpp \
        OnLoad.cpp \
        Common.cpp
		
LOCAL_DISABLE_FORMAT_STRING_CHECKS := true
LOCAL_MODULE    := unipano
include $(BUILD_SHARED_LIBRARY)
