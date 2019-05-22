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
APP=simple-capture
APP_AUDIO=simple-capture-audioOnly
APP_LIBUSB=simple-capture-libusb

BASEDIR=../..
AUDIO?=alsa
VIDEO?=v4l2
PLUGINS=alert custom_control
LIBMXUVC=$(BASEDIR)/lib/libmxuvc.a

-include $(BASEDIR)/src/video/${VIDEO}/ld.mk
-include $(BASEDIR)/src/audio/${AUDIO}/ld.mk
-include $(foreach plugin,$(PLUGINS),$(BASEDIR)/src/plugins/$(plugin)/ld.mk)

CFLAGS+=-ggdb -Werror
CFLAGS+=-I$(BASEDIR)/include -I$(BASEDIR)/src/common
CFLAGS+=-D'VIDEO_BACKEND="$(VIDEO)"' -D'AUDIO_BACKEND="$(AUDIO)"'

all: $(APP) $(APP_AUDIO) $(APP_LIBUSB)

$(APP): $(LIBMXUVC) $(APP).o
	$(CC) $(LDFLAGS) $(APP).o $(LIBMXUVC) $(LDLIBS) -o $(APP)
$(APP_AUDIO): $(LIBMXUVC) $(APP_AUDIO).o
	$(CC) $(LDFLAGS) $(APP_AUDIO).o $(LIBMXUVC) $(LDLIBS) -o $(APP_AUDIO)
$(APP_LIBUSB): $(LIBMXUVC) $(APP_LIBUSB).o
	$(CC) $(LDFLAGS) $(APP_LIBUSB).o $(LIBMXUVC) $(LDLIBS) -o $(APP_LIBUSB)

.c.o:
	$(CC) $(CFLAGS) -c $<

$(LIBMXUVC):
	$(MAKE) -C $(BASEDIR) \
		VIDEO=$(VIDEO) \
		AUDIO=$(AUDIO) \
		PLUGINS="$(PLUGINS)"
clean:
	rm -f $(APP).o $(APP_AUDIO).o $(APP_LIBUSB).o $(APP) $(LIBMXUVC) $(APP_AUDIO) $(APP_LIBUSB)
