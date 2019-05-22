APP_MODULES := libMetaVideo

APP_CFLAGS += -Wno-error=format-security
APP_STL := stlport_static
#APP_ABI := armeabi-v7a x86
APP_ABI := arm64-v8a armeabi armeabi-v7a x86 mips mips64 x86_64