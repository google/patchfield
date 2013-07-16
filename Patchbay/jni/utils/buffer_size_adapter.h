/*
 * An adapter that allows audio modules to operate at buffer sizes that differ
 * from the buffer size of the Patchbay service.
 */
#ifndef __BUFFER_SIZE_ADAPTER_H__
#define __BUFFER_SIZE_ADAPTER_H__

#include "audio_module.h"

/*
 * Abstract data type representing a buffer size adapter.
 */
typedef struct _buffer_size_adapter buffer_size_adapter;

/*
 * Similar to am_create (see audio_module.h).
 */
buffer_size_adapter *bsa_create(
    int version, int token, int index,
    int host_buffer_frames, int user_buffer_frames,
    int input_channels, int output_channels,
    audio_module_process_t user_process, void *user_context);

/*
 * Similar to am_release (see audio_module.h).
 */
void bsa_release(buffer_size_adapter *adapter);

/*
 * Returns the audio module runner instance that's backing the given adapter.
 */
audio_module_runner *bsa_get_runner(buffer_size_adapter *adapter);

#endif
