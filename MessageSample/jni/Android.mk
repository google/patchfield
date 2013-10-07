# Note that NDK_MODULE_PATH must contain the patchfield parent directory. The
# makefile in PatchfieldPcmSample implicitly takes care of this.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := message
LOCAL_LDLIBS := -llog
LOCAL_SRC_FILES := message.c
LOCAL_STATIC_LIBRARIES := audiomodule tinyosc
include $(BUILD_SHARED_LIBRARY)
$(call import-module,Patchfield/jni)
