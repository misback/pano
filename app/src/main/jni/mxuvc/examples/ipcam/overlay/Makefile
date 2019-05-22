#===============================================================================
# 
# The content of this file or document is CONFIDENTIAL and PROPRIETARY
# to GEO Semiconductor.  It is subject to the terms of a License Agreement 
# between Licensee and GEO Semiconductor, restricting among other things,
# the use, reproduction, distribution and transfer.  Each of the embodiments,
# including this information and any derivative work shall retain this 
# copyright notice.
# 
# Copyright 2013-2016 GEO Semiconductor, Inc.
# All rights reserved.
#
# 
#===============================================================================

APPIMAGE=overlay_image
APPTEXT=overlay_text
APPTEXTGPU=overlay_text_gpu
APPPRIVACY=overlay_privacy
APPPRIVACYNIDX=overlay_privacy_multi_idx
APPPRIVACYCMDALPHA=download_compressed_alpha
BASEDIR=../../..
LIBMXUVC=libmxuvc
#LIB_NAME=mxuvc
AUDIO?=alsa
VIDEO?=v4l2
PLUGINS?=overlay custom_control

SRC=$(BASEDIR)/src/common/common.c $(BASEDIR)/src/common/debug.c $(BASEDIR)/src/common/qbox.c \
    $(wildcard $(BASEDIR)/src/video/${VIDEO}/*.c) \
    $(wildcard $(BASEDIR)/src/audio/${AUDIO}/*.c) \
    $(foreach plugin,$(PLUGINS),$(wildcard $(BASEDIR)/src/plugins/$(plugin)/*.c))

-include $(BASEDIR)/src/video/${VIDEO}/ld.mk
-include $(BASEDIR)/src/audio/${AUDIO}/ld.mk
-include $(foreach plugin,$(PLUGINS),$(BASEDIR)/src/plugins/$(plugin)/ld.mk)

CFLAGS+=-ggdb
CFLAGS+=-I$(BASEDIR)/include -I$(BASEDIR)/src/common \
	-I$(BASEDIR)/src/video/${VIDEO} -I$(BASEDIR)/src/audio/${AUDIO} \
	-I$(BASEDIR)/src/plugins/alert -I$(BASEDIR)/src/plugins/overlay \
	-I$(GEOSW_ROOT)/thirdparty/installed/i686-linux/include \
	-I$(BASEDIR)/src/common/libskypeecxu

CFLAGS+=-D'VIDEO_BACKEND="$(VIDEO)"' -D'AUDIO_BACKEND="$(AUDIO)"'
LDLIBS+=-L$(GEOSW_ROOT)/thirdparty/installed/i686-linux/lib

OBJ=$(patsubst %.c,%.o,$(SRC))

all: $(APPTEXT) $(APPTEXTGPU) $(APPIMAGE) $(APPPRIVACY) $(APPPRIVACYNIDX) $(APPPRIVACYCMDALPHA)

$(APPTEXT): $(LIBMXUVC).a $(APPTEXT).o
	$(CC) $(LDFLAGS) $(APPTEXT).o $(LIBMXUVC).a $(LDLIBS) -o $(APPTEXT)
$(APPTEXTGPU): $(LIBMXUVC).a $(APPTEXTGPU).o
	$(CC) $(LDFLAGS) $(APPTEXTGPU).o $(LIBMXUVC).a $(LDLIBS) -o $(APPTEXTGPU)	
$(APPIMAGE): $(LIBMXUVC).a $(APPIMAGE).o
	$(CC) $(LDFLAGS) $(APPIMAGE).o $(LIBMXUVC).a $(LDLIBS) -o $(APPIMAGE)
$(APPPRIVACY): $(LIBMXUVC).a $(APPPRIVACY).o
	$(CC) $(LDFLAGS) $(APPPRIVACY).o $(LIBMXUVC).a $(LDLIBS) -o $(APPPRIVACY)
$(APPPRIVACYNIDX): $(LIBMXUVC).a $(APPPRIVACYNIDX).o
	$(CC) $(LDFLAGS) $(APPPRIVACYNIDX).o $(LIBMXUVC).a $(LDLIBS) -o $(APPPRIVACYNIDX)
$(APPPRIVACYCMDALPHA): $(LIBMXUVC).a $(APPPRIVACYCMDALPHA).o
	$(CC) $(LDFLAGS) $(APPPRIVACYCMDALPHA).o $(LIBMXUVC).a $(LDLIBS) -o $(APPPRIVACYCMDALPHA)
.c.o:
	$(CC) $(CFLAGS) -c $<

$(LIBMXUVC).a: $(OBJ)
	$(MAKE) -C $(BASEDIR) AUDIO=$(AUDIO) VIDEO=$(VIDEO) PLUGINS="$(PLUGINS)"
	cp $(BASEDIR)/lib/libmxuvc_v4l2_alsa_allplugins_trace1.a libmxuvc.a
clean:
	rm -f $(APPTEXT).o $(APPTEXT) $(APPTEXTGPU).o $(APPTEXTGPU) $(APPIMAGE).o $(APPIMAGE) $(APPPRIVACY).o $(APPPRIVACY) $(APPPRIVACYNIDX) $(APPPRIVACYCMDALPHA) $(LIBMXUVC).a
