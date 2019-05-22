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
INSTALLED_DIR	:= $(GEOSW_ROOT)/installed
INSTALLED_DOC_D	:= $(INSTALLED_DIR)/doc
INSTALLED_DOC_F	:=					\
		$(INSTALLED_DOC_D)/html \
		$(INSTALLED_DOC_D)/mxuvc.html

DOXYGEN		:= doxygen

# SVN_VER should contain eithr output of svnversion or 'exported'
ifeq ($(filter clean%,$(MAKECMDGOALS)),)
SVN_VER 	:= $(shell 2>&1 svnversion | sed 's,.*Command not found.*,exported,; s,.*Unversioned.*,exported,; s,.*xported.*,exported,;' )
else
SVN_VER		:= 0
endif

all:
	sed -e "s#(INPUTS)#$(GEOSW_ROOT)/condorsw/host/lib/mxuvc/include/mxuvc.h \
	$(GEOSW_ROOT)/condorsw/host/lib/mxuvc/include/mxuvc_dewarp.h \
	$(GEOSW_ROOT)/condorsw/host/lib/mxuvc/doc/ipcam/resource/intro.txt#g" \
	-e "s#(LOGO_FILE_PATH)#$(GEOSW_ROOT)/condorsw/host/lib/mxuvc/doc/ipcam/resource/geo_logo.jpg#g" \
	-e "s/(REL_VERSION_STR)/$(SVN_VER)/g" DoxygenConfiguration | $(DOXYGEN) -
install: 
installdoc: $(INSTALLED_DOC_F)

$(INSTALLED_DOC_D)/%: %
	mkdir -p $(@D)
	cp -a $(<) $(@)

clean:
distclean:

#===============================================================================
# vi: set filetype=make shiftwidth=8 tabstop=8 softtabstop=8 noexpandtab nosmarttab:
#===============================================================================
