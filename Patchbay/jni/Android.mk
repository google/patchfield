LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := shared_memory_utils
LOCAL_LDLIBS := -llog
LOCAL_SRC_FILES := shared_memory_utils.c shared_memory_internal.c
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := patchbay
LOCAL_LDLIBS := -lOpenSLES -llog
LOCAL_SRC_FILES := patchbay.c shared_memory_internal.c audio_module_internal.c \
  simple_barrier.c opensl_stream/opensl_stream.c
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := audiomodule
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_EXPORT_LDLIBS := -lOpenSLES -llog
LOCAL_SRC_FILES := audio_module.c audio_module_internal.c simple_barrier.c \
	shared_memory_internal.c opensl_stream/opensl_stream.c
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := buffersizeadapter
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_SRC_FILES := utils/buffer_size_adapter.c
LOCAL_STATIC_LIBRARIES := audiomodule
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := javamodule
LOCAL_LDLIBS := -llog
LOCAL_SRC_FILES := javamodule.c
LOCAL_STATIC_LIBRARIES := audiomodule
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := lowpass
LOCAL_LDLIBS := -llog
LOCAL_SRC_FILES := samples/lowpass.c
LOCAL_STATIC_LIBRARIES := audiomodule
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := identity
LOCAL_LDLIBS := -llog
LOCAL_SRC_FILES := samples/identity.c
LOCAL_STATIC_LIBRARIES := audiomodule
include $(BUILD_SHARED_LIBRARY)
