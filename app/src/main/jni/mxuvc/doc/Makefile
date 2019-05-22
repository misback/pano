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
# @(#) $Id: Makefile 57887 2016-04-29 10:50:27Z bsmith $
#===============================================================================

SUB_MODS	= ipcam

all: $(SUB_MODS:%=%/all)
install: 
installdoc: $(SUB_MODS:%=%/installdoc)

%/all:
	$(MAKE) $(@F) -C $(@D)

%/installdoc:
	$(MAKE) $(@F) -C $(@D)

#===============================================================================
# vi: set filetype=make shiftwidth=8 tabstop=8 softtabstop=8 noexpandtab nosmarttab:
#===============================================================================
