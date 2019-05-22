# $Id: Makefile 60280 2017-01-09 12:32:42Z venkatesh $

LIB_INVARIANT_NAME	:= mxuvc
LIB_NAME		:= mxuvc
UNAME 			:= $(shell uname)

INSTALLED_DIR		:= $(GEOSW_ROOT)/installed
INSTALLED_LIB_D		:= $(INSTALLED_DIR)/lib

# SVN_VER should contain eithr output of svnversion or 'exported'
ifeq ($(filter clean%,$(MAKECMDGOALS)),)
SVN_VER 	:= $(shell 2>&1 svnversion | sed 's,.*Command not found.*,exported,; s,.*Unversioned.*,exported,; s,.*xported.*,exported,;' )
else
SVN_VER		:= 0
endif

#
# Default backends
AUDIO		?= alsa
VIDEO		?= v4l2
PLUGINS		?=		\
		alert		\
		overlay		\
		ispcfg		\
		custom_control

SRC		=									\
		src/common/common.c							\
		src/common/debug.c							\
		src/common/qbox.c							\
		src/common/yuvutil.c							\
		$(if $(VIDEO), $(wildcard src/video/${VIDEO}/*.c))			\
		$(if $(AUDIO), $(wildcard src/audio/${AUDIO}/*.c))			\
		$(foreach plugin,$(PLUGINS),$(wildcard src/plugins/$(plugin)/*.c))
OBJ		= $(addprefix $(BUILD_DIR), $(patsubst %.c,%.o,$(SRC)))
SUB_MODS	= doc

STATIC_LIBRARY_NAMES	=				\
			lib$(LIB_NAME).a		\
			lib$(LIB_INVARIANT_NAME).a
SHARED_LIBRARY_NAMES	=				\
			lib$(LIB_NAME).so		\
			lib$(LIB_INVARIANT_NAME).so

ifneq (,$(findstring arm,$(shell $(CC) -dumpmachine)))
#ARM
CFLAGS          +=                                                                      \
                -fPIC                                                                   \
                -ggdb                                                                   \
                -fno-common                                                             \
                -fstrict-overflow                                                       \
                -Wno-empty-body                                                         \
                -Wno-unused-parameter                                                   \
                -Iinclude                                                               \
                -Isrc/common                                                            \
                $(if $(VIDEO), -Isrc/video/${VIDEO})                                    \
                $(if $(AUDIO), -Isrc/audio/${AUDIO})                                    \
                $(foreach plugin,$(PLUGINS),-Isrc/plugins/$(plugin))                    \
                -I$(GEOSW_ROOT)/thirdparty/installed/i686-linux/include/                \
                $(USER_CFLAGS)
else
#x86, 64, mips...
CFLAGS		+=									\
		-fPIC									\
		-ggdb									\
		-fno-common								\
		-fstrict-overflow							\
		-Wall									\
		-Werror									\
		-Wcast-align								\
		-Wcast-qual								\
		-Wextra									\
		-Wno-empty-body								\
		-Wno-unused-parameter							\
		-Wshadow								\
		-Wwrite-strings								\
		-Wswitch-default							\
		-Iinclude								\
		-Isrc/common								\
		$(if $(VIDEO), -Isrc/video/${VIDEO})					\
		$(if $(AUDIO), -Isrc/audio/${AUDIO})					\
		$(foreach plugin,$(PLUGINS),-Isrc/plugins/$(plugin))			\
		-I$(GEOSW_ROOT)/thirdparty/installed/i686-linux/include/		\
		$(USER_CFLAGS)
endif

-include src/video/${VIDEO}/extra.mk
-include src/audio/${AUDIO}/extra.mk
-include $(foreach plugin,$(PLUGINS),src/plugins/$(plugin)/extra.mk)

#-include src/video/${VIDEO}/ld.mk
#-include src/audio/${AUDIO}/ld.mk
#-include $(foreach plugin,$(PLUGINS),src/plugins/$(plugin)/ld.mk)


FLAVORS		= 					\
		lib_$(VIDEO)_trace0				\
		lib_$(VIDEO)_alsa_customcontrol_trace0	\
		lib_$(VIDEO)_alsa_allplugins_trace1		\
		lib_$(VIDEO)_alsa_allplugins_trace0

all:: 		$(SUB_MODS:%=%/all)     $(addsuffix /build_libray,    $(FLAVORS)) install
install::	$(SUB_MODS:%=%/install)	$(addsuffix /install_library, $(FLAVORS))
installdoc::	$(SUB_MODS:%=%/installdoc)	

%/all:
	$(MAKE) $(@F) -C $(@D)

%/install:
	$(MAKE) $(@F) -C $(@D)

%/installdoc:
	$(MAKE) $(@F) -C $(@D)

build_libray:		static_library
build_libray:		shared_library

lib_$(VIDEO)_trace0/%:
	$(MAKE) $(@F)						\
		LIB_NAME=$(patsubst lib_%,$(LIB_NAME)_%,$(@D))	\
		BUILD_DIR=build_dir_$(@D)/			\
		USER_CFLAGS=-D'MXUVC_TRACE_LEVEL=0' 		\
		VIDEO=$(VIDEO)			
lib_$(VIDEO)_alsa_customcontrol_trace0/%:
	$(MAKE) $(@F)						\
		LIB_NAME=$(patsubst lib_%,$(LIB_NAME)_%,$(@D))	\
		BUILD_DIR=build_dir_$(@D)/			\
		USER_CFLAGS=-D'MXUVC_TRACE_LEVEL=0' 		\
		VIDEO=$(VIDEO)					\
		AUDIO=alsa					\
		PLUGINS=custom_control
lib_$(VIDEO)_alsa_allplugins_trace1/%:
	$(MAKE) $(@F)						\
		LIB_NAME=$(patsubst lib_%,$(LIB_NAME)_%,$(@D))	\
		BUILD_DIR=build_dir_$(@D)/			\
		USER_CFLAGS=-D'MXUVC_TRACE_LEVEL=1' 		\
		VIDEO=$(VIDEO)					\
		AUDIO=alsa					\
		PLUGINS="$(PLUGINS)"
lib_$(VIDEO)_alsa_allplugins_trace0/%:
	$(MAKE) $(@F)						\
		LIB_NAME=$(patsubst lib_%,$(LIB_NAME)_%,$(@D))	\
		BUILD_DIR=build_dir_$(@D)/			\
		USER_CFLAGS=-D'MXUVC_TRACE_LEVEL=0' 		\
		VIDEO=$(VIDEO)					\
		AUDIO=alsa					\
		PLUGINS="$(PLUGINS)"



static_library:		$(addprefix lib/,                 $(STATIC_LIBRARY_NAMES) )
install_library:	$(addprefix $(INSTALLED_LIB_D)/,  $(STATIC_LIBRARY_NAMES) )

ifneq ($(LIB_NAME),$(LIB_INVARIANT_NAME))
lib/lib$(LIB_NAME).a:	$(OBJ)
	mkdir -p $(@D)
	$(AR) rc $(@) $(OBJ)
endif

lib/lib$(LIB_INVARIANT_NAME).a:
	mkdir -p $(@D)
	cd $(@D) && ln --force --symbolic libmxuvc_$(VIDEO)_alsa_allplugins_trace1.a  $(@F)

LIBUSB=1
ifeq ($(UNAME), Linux)
ifeq ($(CC), cc) #chceck if its x86 target
ifeq ($(wildcard /etc/fedora-release),)
else
ifeq ($(wildcard /usr/include/libusb-1.0/libusb.h),)
	LIBUSB=0
else
endif
endif
endif

CFLAGS		+=				\
		-pipe				\
		-fno-exceptions			\
		-Wl,-z,now			\
		-Wl,-z,relro			\
		-Wno-array-bounds		\
		-Wno-deprecated			\
		-Wno-ignored-qualifiers		\
		-Wno-unused-function		\
		-Wno-unused-label		\
		-Wno-unused-parameter		

ifeq (,$(findstring arm,$(shell $(CC) -dumpmachine)))
CFLAGS		+=				\
		-Wformat			\
		-Wformat-nonliteral		\
		-Wformat-security		\
		-Winit-self			\
		-Wmultichar			\
		-Wpointer-arith			\
		-Wreturn-type			\
		-Wsign-compare			\
		-Wuninitialized				
endif

shared_library: 	$(addprefix lib/,                 $(SHARED_LIBRARY_NAMES) )
install_library:	$(addprefix $(INSTALLED_LIB_D)/,  $(SHARED_LIBRARY_NAMES) )
lib/lib$(LIB_NAME).so:	$(OBJ)
	mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(OBJ) -shared -Wl,-soname,lib$(LIB_NAME).so $(LDLIBS) -o $(@)
ifneq	($(LIB_NAME),$(LIB_INVARIANT_NAME))
lib/lib$(LIB_INVARIANT_NAME).so:
	mkdir -p $(@D)
	cd $(@D) && ln --force --symbolic libmxuvc_$(VIDEO)_alsa_allplugins_trace1.so $(@F)
endif	# ($(LIB_NAME),$(LIB_INVARIANT_NAME))
endif
ifeq ($(UNAME), Darwin)
shared_library:			lib$(LIB_NAME).1.dylib
lib$(LIB_NAME).1.dylib: $(OBJ)
	$(CC) $(LDFLAGS) -dynamiclib -Wl,-headerpad_max_install_names,-undefined,\
		dynamic_lookup,-compatibility_version,1.0,-current_version,1.0,\
		-install_name,/usr/local/lib/lib$(LIB_NAME).1.dylib \
		-o $(@) $(LDLIBS) $(OBJ)
endif

$(INSTALLED_LIB_D)/%:	lib/%
	mkdir -p $(@D)
	cp -uvd $(<) $(@)

$(BUILD_DIR)%.o :	%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f lib$(LIB_NAME).1.dylib
	rm -fr lib
	rm -fr build_dir_*

distclean: clean
	rm -f $(addprefix $(INSTALLED_LIB_D)/, $(SHARED_LIBRARY_NAMES))

.PHONY:	lib
.PHONY:	install
.PHONY:	installdoc
.PHONY:	clean
.PHONY:	distclean
.PHONY:	all
.PHONY:	static_library
.PHONY:	shared_library
