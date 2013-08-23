/*
 * Copyright 2013 Google Inc. All Rights Reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

/*
 * An adapter that allows audio modules to operate at buffer sizes that differ
 * from the buffer size of the Patchfield service.
 */
#ifndef __BUFFER_SIZE_ADAPTER_H__
#define __BUFFER_SIZE_ADAPTER_H__

#include "audio_module.h"

/*
 * Abstract data type representing a buffer size adapter.
 */
typedef struct _buffer_size_adapter buffer_size_adapter;

/*
 * Constructor; calls am_configure internally, and so audio modules using
 * this utility will not need to call am_configure themselves.
 */
buffer_size_adapter *bsa_create(
    void *handle, int host_buffer_frames, int user_buffer_frames,
    int input_channels, int output_channels,
    audio_module_process_t user_process, void *user_context);

/*
 * Releases all resources associated with this adapter.
 */
void bsa_release(buffer_size_adapter *adapter);

#endif
