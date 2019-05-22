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

#===============================================================================
# @(#) $Id: Makefile 59087 2016-08-08 12:46:41Z ankurm $
#===============================================================================
UNAME		= $(shell uname)

CFLAGS		+=							\
		-ggdb							\
		-Wno-empty-body						\
		-Wno-unused-parameter 					\
		-I$(GEOSW_ROOT)/thirdparty/installed/i686-linux/include
ifeq (,$(findstring arm,$(shell $(CC) -dumpmachine)))
CFLAGS		+=							\
		-Wall							\
		-Werror							\
		-Wcast-align						\
		-Wcast-qual						\
		-Wextra							\
		-Wshadow						\
		-Wwrite-strings						\
		-Wswitch-default					
endif

ifeq ($(UNAME), Linux)
CFLAGS		+=				\
		-Wno-array-bounds		\
		-Wno-ignored-qualifiers		
LIBRARY_NAMES	=		\
		libmxcam.a	
endif
ifeq ($(UNAME), Darwin)
LIBRARY_NAMES	=		\
		libmxcam.a	\
		libmxcam.1.dylib
endif

INSTALLED_DIR		:= $(GEOSW_ROOT)/installed
INSTALLED_LIB_D		:= $(INSTALLED_DIR)/lib
INSTALLED_LIB_F		:= $(addprefix $(INSTALLED_LIB_D)/, $(LIBRARY_NAMES))

all:		$(LIBRARY_NAMES) 
install:	$(INSTALLED_LIB_F)

ldmap.o: ldmap.c ldmap.h
	$(CC) $(CFLAGS) -o ldmap.o -c ldmap.c
xmodem.o: xmodem.c xmodem.h
	$(CC) $(CFLAGS) -o xmodem.o -c xmodem.c
libmxcam.o: libmxcam.c libmxcam.h 
	$(CC) $(CFLAGS) -o libmxcam.o -c libmxcam.c 
libmxcam.a: libmxcam.o xmodem.o ldmap.o
	$(AR) rc libmxcam.a libmxcam.o xmodem.o ldmap.o

libmxcam.1.dylib: libmxcam.o xmodem.o ldmap.o
	$(CC)	\
		-dynamiclib						\
		-Wl,-headerpad_max_install_names			\
		-Wl,-undefined,dynamic_lookup				\
		-Wl,-compatibility_version,1.0				\
		-Wl,-current_version,1.0				\
		-Wl,-install_name,/usr/local/lib/libmxcam.1.dylib	\
		-o libmxcam.1.dylib					\
		libmxcam.o xmodem.o ldmap.o

clean: clean-shared
	rm -rf libmxcam.o libmxcam.a xmodem.o ldmap.o

clean-shared:
	rm -rf libmxcam.1.dylib

$(INSTALLED_LIB_D)/%:	%
	mkdir -p $(@D)
	cp -uvd $(<) $(@)

.PHONY: all shared clean clean-shared
