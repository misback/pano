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
APPIMAGE=ldmap
BASEDIR=../../..
LIBMXUVC=libmxuvc
#LIB_NAME=mxuvc
AUDIO?=alsa
VIDEO?=v4l2
PLUGINS?=pi_ldmap custom_control

SRC=$(BASEDIR)/src/common/common.c $(BASEDIR)/src/common/debug.c $(BASEDIR)/src/common/qbox.c \
    $(wildcard $(BASEDIR)/src/video/${VIDEO}/*.c) \
    $(wildcard $(BASEDIR)/src/audio/${AUDIO}/*.c) \
    $(foreach plugin,$(PLUGINS),$(wildcard $(BASEDIR)/src/plugins/$(plugin)/*.c)) \
	ldmap.c

-include $(BASEDIR)/src/video/${VIDEO}/ld.mk
-include $(BASEDIR)/src/audio/${AUDIO}/ld.mk
-include $(foreach plugin,$(PLUGINS),$(BASEDIR)/src/plugins/$(plugin)/ld.mk)

CFLAGS+=-ggdb
CFLAGS+=-I$(BASEDIR)/include -I$(BASEDIR)/src/common \
	-I$(BASEDIR)/src/video/${VIDEO} -I$(BASEDIR)/src/audio/${AUDIO} \
	-I$(BASEDIR)/src/plugins/alert -I$(BASEDIR)/src/plugins/pi_ldmap \
	-I$(GEOSW_ROOT)/thirdparty/installed/i686-linux/include \
	-I$(BASEDIR)/src/common/libskypeecxu

CFLAGS+=-D'VIDEO_BACKEND="$(VIDEO)"' -D'AUDIO_BACKEND="$(AUDIO)"'
LDLIBS+=-L$(GEOSW_ROOT)/thirdparty/installed/i686-linux/lib

OBJ=$(patsubst %.c,%.o,$(SRC))

all: $(APPIMAGE)

$(APPIMAGE): $(LIBMXUVC).a $(APPIMAGE).o
	$(CC) $(LDFLAGS) $(APPIMAGE).o $(LIBMXUVC).a $(LDLIBS) -o $(APPIMAGE)
.c.o:
	$(CC) $(CFLAGS) -c $<

$(LIBMXUVC).a: $(OBJ)
	$(MAKE) -C $(BASEDIR) AUDIO=$(AUDIO) VIDEO=$(VIDEO) PLUGINS="$(PLUGINS)"
	cp $(BASEDIR)/lib/libmxuvc_v4l2_alsa_allplugins_trace1.a libmxuvc.a
clean:
	rm -f *.o $(APPIMAGE) $(LIBMXUVC).a
