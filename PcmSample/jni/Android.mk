# Note that NDK_MODULE_PATH must contain the patchfield parent directory. The
# makefile in PcmSample implicitly takes care of this.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := pcmsource
LOCAL_LDLIBS := -llog
LOCAL_SRC_FILES := pcmsource.c
LOCAL_STATIC_LIBRARIES := audiomodule
include $(BUILD_SHARED_LIBRARY)
$(call import-module,Patchfield/jni)
