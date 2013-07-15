# Note that NDK_MODULE_PATH must contain the patchbay parent directory. The
# makefile in PatchbayPd implicitly takes care of this.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := pdmodule
LOCAL_LDLIBS := -llog
LOCAL_SRC_FILES := pdmodule.c
LOCAL_STATIC_LIBRARIES := audiomodule buffersizeadapter
LOCAL_SHARED_LIBRARIES := pd pdnativeopensl
include $(BUILD_SHARED_LIBRARY)
$(call import-module,Patchbay/jni)
