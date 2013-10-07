# Note that NDK_MODULE_PATH must contain the patchfield parent directory. The
# makefile in PatchfieldPcmSample implicitly takes care of this.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := message
LOCAL_C_INCLUDES := tinyosc/src
LOCAL_LDLIBS := -llog
LOCAL_SRC_FILES := message.c tinyosc/src/tinyosc.c tinyosc/src/pattern.c
LOCAL_STATIC_LIBRARIES := audiomodule
include $(BUILD_SHARED_LIBRARY)
$(call import-module,Patchfield/jni)
