LOCAL_PATH := $(call my-dir)
MAIN_LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := jbro

LOCAL_CFLAGS := -fpermissive -DLOG_TAG=\"jbro\"
LOCAL_CPPFLAGS := -std=c++11 -D __cplusplus=201103L

LOCAL_C_INCLUDES += $(MAIN_LOCAL_PATH)

LOCAL_SRC_FILES := jbro.cpp \
                   Substrate/hde64.c \
                   Substrate/SubstrateDebug.cpp \
                   Substrate/SubstrateHook.cpp \
                   Substrate/SubstratePosixMemory.cpp \

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
