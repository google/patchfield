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

#include "audio_module.h"

#include "internal/audio_module_internal.h"

void am_configure(void *handle, audio_module_process_t process, void *context) {
  audio_module_runner *amr = (audio_module_runner *) handle;
  amr->process = process;
  amr->context = context;
}

int am_next_message(void *handle, am_message *message) {
  audio_module_runner *amr = (audio_module_runner *) handle;
  ptrdiff_t rp = *(ptrdiff_t *)ami_get_message_buffer(amr->shm_ptr,
      ami_get_read_ptr_offset);
  ptrdiff_t wp = *(ptrdiff_t *)ami_get_message_buffer(amr->shm_ptr,
      ami_get_write_ptr_offset);
  int size = message->size;
  if (size & 0x03) {
    size += 4 - (size & 0x03);
  }
  ptrdiff_t dp = (message->data == NULL) ? rp : ((message->data -
        ((char *)ami_get_message_buffer(amr->shm_ptr, 0))) + size);
  if (dp == wp) return -1;  // Reached end of messages.
  if (*(int *)ami_get_message_buffer(amr->shm_ptr, dp) == 0) {
    dp = ami_get_data_offset;
  }
  if (dp == wp) return -1;
  char *p = ami_get_message_buffer(amr->shm_ptr, dp);
  message->size = *(int *)p;
  message->data = p + 4;
  return 0;
}
