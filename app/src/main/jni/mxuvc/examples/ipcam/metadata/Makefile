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

METADEWARPAPP=dewarp_metadata
METASENSORAPP=sensor_metadata
METAGYROAPP=gyro_metadata

BASEDIR=../../..
AUDIO?=alsa
VIDEO?=v4l2
LIBMXUVC=$(BASEDIR)/lib/libmxuvc.a
PLUGINS=custom_control

-include $(BASEDIR)/src/video/${VIDEO}/ld.mk
-include $(BASEDIR)/src/audio/${AUDIO}/ld.mk
-include $(foreach plugin,$(PLUGINS),$(BASEDIR)/src/plugins/$(plugin)/ld.mk)

CFLAGS+=-ggdb 
CFLAGS+=-I$(BASEDIR)/include -I$(BASEDIR)/src/common
CFLAGS+=-D'VIDEO_BACKEND="$(VIDEO)"' -D'AUDIO_BACKEND="$(AUDIO)"'
LDLIBS+=-L$(GEOSW_ROOT)/thirdparty/installed/i686-linux/lib

all: $(METADEWARPAPP) $(METASENSORAPP) $(METAGYROAPP)

$(METADEWARPAPP): $(LIBMXUVC) $(METADEWARPAPP).o
	$(CC) $(LDFLAGS) $(METADEWARPAPP).o $(LIBMXUVC) $(LDLIBS) -o $(METADEWARPAPP)

$(METASENSORAPP): $(LIBMXUVC) $(METASENSORAPP).o
	$(CC) $(LDFLAGS) $(METASENSORAPP).o $(LIBMXUVC) $(LDLIBS) -o $(METASENSORAPP)

$(METAGYROAPP): $(LIBMXUVC) $(METAGYROAPP).o
	$(CC) $(LDFLAGS) $(METAGYROAPP).o $(LIBMXUVC) $(LDLIBS) -o $(METAGYROAPP)

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
	rm -f $(METADEWARPAPP).o $(METADEWARPAPP) $(METASENSORAPP).o $(METASENSORAPP) $(METAGYROAPP).o $(METAGYROAPP)
