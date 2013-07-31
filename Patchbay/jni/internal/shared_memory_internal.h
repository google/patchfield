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
 * Tools for handling shared memory with ashmem. Most functions simply wrap
 * basic system calls, except for smi_{send,receive}, which serve to pass the
 * ashmem file descriptor across process boundaries using Unix domain sockets.
 */

#ifndef __SHARED_MEMORY_INTERNAL_H__
#define __SHARED_MEMORY_INTERNAL_H__

#include <stddef.h>

int smi_create();
void *smi_map(int fd);
int smi_unmap(void *p);
int smi_lock(void *p);
int smi_unlock(void *p);
int smi_protect(void *p, size_t n);
int smi_send(int fd);
int smi_receive();
long smi_get_size();

#endif
