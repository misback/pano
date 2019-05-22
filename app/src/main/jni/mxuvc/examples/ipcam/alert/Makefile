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

APP=alert
APP_AUDIO=alert_audio
APP_I2C=alert_i2c
#Following example to be used for OPT3001 ALS
APP_I2C_ALS=alert_i2c_als
APP_GPIO=alert_gpio
BASEDIR=../../..
LIBMXUVC=$(BASEDIR)/lib/libmxuvc.a
AUDIO?=alsa
VIDEO?=v4l2
PLUGINS=alert custom_control

-include $(BASEDIR)/src/video/${VIDEO}/ld.mk
-include $(BASEDIR)/src/audio/${AUDIO}/ld.mk
-include $(foreach plugin,$(PLUGINS),$(BASEDIR)/src/plugins/$(plugin)/ld.mk)

CFLAGS+=-ggdb 
CFLAGS+=-I$(BASEDIR)/include -I$(BASEDIR)/src/common
CFLAGS+=-D'VIDEO_BACKEND="$(VIDEO)"' -D'AUDIO_BACKEND="$(AUDIO)"'
LDLIBS+=-L$(GEOSW_ROOT)/thirdparty/installed/i686-linux/lib

all: $(APP) $(APP_AUDIO) $(APP_I2C) $(APP_I2C_ALS) $(APP_GPIO)

$(APP): $(LIBMXUVC) $(APP).o
	$(CC) $(LDFLAGS) $(APP).o $(LIBMXUVC) $(LDLIBS) -o $(APP)
$(APP_AUDIO): $(LIBMXUVC) $(APP_AUDIO).o
	$(CC) $(LDFLAGS) $(APP_AUDIO).o $(LIBMXUVC) $(LDLIBS) -o $(APP_AUDIO)
$(APP_I2C):  $(LIBMXUVC) $(APP_I2C).o
	$(CC) $(LDFLAGS) $(APP_I2C).o  $(LIBMXUVC) $(LDLIBS) -o $(APP_I2C)
$(APP_I2C_ALS):  $(LIBMXUVC) $(APP_I2C_ALS).o
	$(CC) $(LDFLAGS) $(APP_I2C_ALS).o  $(LIBMXUVC) $(LDLIBS) -o $(APP_I2C_ALS)
$(APP_GPIO):  $(LIBMXUVC) $(APP_GPIO).o
	$(CC) $(LDFLAGS) $(APP_GPIO).o  $(LIBMXUVC) $(LDLIBS) -o $(APP_GPIO)

.c.o:
	$(CC) $(CFLAGS) -c $<

# ------------------------------------------------------------------------------
# FIXME:
# DO NOT COPY THIS RULE.
#
# This Makefile should ideally use one of the several pre-built
# $(BASEDIR)/lib/libmxuvc*.a and should NOT contain any rule to build a libmxuvc*.a.
#
# In a less ideal case it should use trigger a default build using the
# command line '$(MAKE) -C $(BASEDIR)' and it should absolutely NOT use
# command line options to alter the libraries built in $(BASEDIR)/lib/.
# ------------------------------------------------------------------------------
$(LIBMXUVC):
	$(MAKE) -C $(BASEDIR) \
		VIDEO=$(VIDEO) \
		AUDIO=$(AUDIO) \
		PLUGINS="$(PLUGINS)"
clean:
	rm -f $(APP).o $(APP) $(LIBMXUVC) $(APP_AUDIO).o $(APP_AUDIO) $(APP_I2C) $(APP_I2C).o \
	$(APP_I2C_ALS) $(APP_I2C_ALS).o $(APP_GPIO) $(APP_GPIO).o
