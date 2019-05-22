######################################################################
# libjpeg-turbo1.4.2.a
######################################################################
LOCAL_PATH := $(call my-dir)/../..
include $(CLEAR_VARS)

CFLAGS := -Werror

LOCAL_MODULE := jpeg-turbo142

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/

LOCAL_EXPORT_C_INCLUDES := \
		$(LOCAL_PATH)/

LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_CFLAGS += -DANDROID_NDK

LOCAL_CFLAGS += -DAVOID_TABLES
LOCAL_CFLAGS += -O3 -fstrict-aliasing
LOCAL_CFLAGS += -fprefetch-loop-arrays


LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	jcapimin.c jcapistd.c jccoefct.c \
	jccolor.c jcdctmgr.c jchuff.c jcinit.c jcmainct.c jcmarker.c \
	jcmaster.c jcomapi.c jcparam.c jcphuff.c jcprepct.c jcsample.c \
	jctrans.c jdapimin.c jdapistd.c jdatadst.c jdatasrc.c \
	jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c jdinput.c jdmainct.c \
	jdmarker.c jdmaster.c jdmerge.c jdphuff.c jdpostct.c \
	jdsample.c jdtrans.c jerror.c jfdctflt.c jfdctfst.c jfdctint.c \
	jidctflt.c jidctfst.c jidctint.c jidctred.c jquant1.c \
	jquant2.c jutils.c jmemmgr.c jmemnobs.c \
	jaricom.c jdarith.c jcarith.c \
	turbojpeg.c transupp.c jdatadst-tj.c jdatasrc-tj.c \

ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_SRC_FILES += simd/jsimd_arm.c simd/jsimd_arm_neon.S

else ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_SRC_FILES += simd/jsimd_arm.c simd/jsimd_arm_neon.S

else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_SRC_FILES += simd/jsimd_arm64.c simd/jsimd_arm64_neon.S
else
LOCAL_SRC_FILES += jsimd_none.c
endif

include $(BUILD_STATIC_LIBRARY)


