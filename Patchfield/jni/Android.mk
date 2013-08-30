LOCAL_PATH := $(call my-dir)

# Public libraries.

include $(CLEAR_VARS)

LOCAL_MODULE := audiomodule
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_EXPORT_CFLAGS := -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
LOCAL_SRC_FILES := internal/audio_module.c
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := buffersizeadapter
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_SRC_FILES := utils/buffer_size_adapter.c
LOCAL_STATIC_LIBRARIES := audiomodule
include $(BUILD_STATIC_LIBRARY)


# Internal libraries.

include $(CLEAR_VARS)

LOCAL_MODULE := audiomoduleinternal
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_EXPORT_CFLAGS := -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
LOCAL_EXPORT_LDLIBS := -lOpenSLES -llog
LOCAL_SRC_FILES := internal/audio_module_internal.c \
	internal/simple_barrier.c internal/shared_memory_internal.c \
	opensl_stream/opensl_stream.c
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := audiomodulejava
LOCAL_STATIC_LIBRARIES := audiomoduleinternal
LOCAL_SRC_FILES := internal/audio_module_java.c
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := patchfield
LOCAL_STATIC_LIBRARIES := audiomoduleinternal
LOCAL_SRC_FILES := internal/patchfield.c
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := shared_memory_utils
LOCAL_LDLIBS := -llog
LOCAL_SRC_FILES := internal/shared_memory_utils.c \
	internal/shared_memory_internal.c
include $(BUILD_SHARED_LIBRARY)
