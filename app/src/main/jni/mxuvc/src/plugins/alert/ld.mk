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
ifndef LINK_LIBUSB
	LINK_LIBUSB=1
	LDLIBS+=-lusb-1.0
endif

ifndef LINK_LIBRT
ifeq ($(shell uname), Linux)
	LINK_LIBRT=1
	LDLIBS+=-lrt
endif
endif

ifndef LINK_PTHREAD
	LINK_PTHREAD=1
	LDLIBS+=-lpthread
endif

ifndef LINK_LIBM
	LINK_LIBM=1
	LDLIBS+=-lm
endif
