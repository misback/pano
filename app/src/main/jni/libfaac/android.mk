LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libfaac

SRC_PATH := ../libfaac
LOCAL_C_INCLUDES :=  $(LOCAL_PATH)

# Add your application source files here...
LOCAL_SRC_FILES :=  aacquant.c \
                    backpred.c  \
	           bitstream.c  \
	           channels.c \
	          fft.c \
		  filtbank.c \
		  frame.c \
		   huffman.c \
		   ltp.c\
		   midside.c \
		   psychkni.c \
		   tns.c      \
		   util.c    \
		   AacEncoder.c \
		   input.c
LOCAL_CFLAGS :=-DHAVE_CONFIG_H
LOCAL_LDLIBS := -llog -lz

include $(BUILD_SHARED_LIBRARY)
