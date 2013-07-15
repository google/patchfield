#ifndef __BUFFER_SIZE_ADAPTER_H__
#define __BUFFER_SIZE_ADAPTER_H__

#include "audio_module.h"

typedef struct _buffer_size_adapter buffer_size_adapter;

buffer_size_adapter *bsa_create(
    int version, int token, int index,
    int host_buffer_frames, int user_buffer_frames,
    int input_channels, int output_channels,
    audio_module_process_t user_process, void *user_context);

void bsa_release(buffer_size_adapter *adapter);

audio_module_runner *bsa_get_runner(buffer_size_adapter *adapter);

#endif
