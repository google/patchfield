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
 * Internal data structures representing audio modules and their connections,
 * as well as some functions that operate on them.
 */

#ifndef __AUDIO_MODULE_INTERNAL_H__
#define __AUDIO_MODULE_INTERNAL_H__

#include "audio_module.h"

#include "simple_barrier.h"

#include <pthread.h>
#include <stddef.h>
#include <time.h>
#include <unistd.h>

#define PATCHFIELD_PROTOCOL_VERSION 6

#define MAX_MODULES 32 
#define MAX_CONNECTIONS 16 

#define MEM_PAGE_SIZE sysconf(_SC_PAGESIZE)
#define BARRIER_OFFSET (MAX_MODULES * sizeof(audio_module) / MEM_PAGE_SIZE + 1)
#define BUFFER_OFFSET \
  (BARRIER_OFFSET + MAX_MODULES * 3 * sizeof(int) / MEM_PAGE_SIZE + 1)

typedef struct {
  int status;  // 0: none; 1: current; 2: slated for deletion
  int in_use;

  int source_index;
  int source_port;
  int sink_port;
} connection;

typedef struct {
  int status;  // 0: none; 1: current; 2: slated for deletion
  int active;
  int in_use;

  int sample_rate;
  int buffer_frames;

  int input_channels;
  ptrdiff_t input_buffer;   // Storing buffers as offsets of type ptrdiff_t
  int output_channels;      // rather than pointers of type float* to render
  ptrdiff_t output_buffer;  // them independent of the shared memory location.

  connection input_connections[MAX_CONNECTIONS];

  struct timespec deadline;
  ptrdiff_t report;
  ptrdiff_t wake;
  ptrdiff_t ready;
} audio_module;

typedef struct {
  int shm_fd;
  void *shm_ptr;
  int index;
  pthread_t thread;
  simple_barrier_t launched;
  int launch_counter;
  int done;
  int timed_out;
  audio_module_process_t process;
  void *context;
} audio_module_runner;

audio_module *ami_get_audio_module(void *p, int index);
float *ami_get_audio_buffer(void *p, ptrdiff_t offset);
simple_barrier_t *ami_get_barrier(void *p, ptrdiff_t offset);
void ami_collect_input(void *p, int index);
audio_module_runner *ami_create(int version, int token, int index);
void ami_release(audio_module_runner *p);
int ami_has_timed_out(audio_module_runner *p);

#endif
