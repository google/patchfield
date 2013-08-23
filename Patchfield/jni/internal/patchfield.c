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

#include "patchfield.h"

#include "audio_module_internal.h"
#include "opensl_stream/opensl_stream.h"
#include "shared_memory_internal.h"
#include "simple_barrier.h"

#include <android/log.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LOGI(...) \
  __android_log_print(ANDROID_LOG_INFO, "patchfield", __VA_ARGS__)
#define LOGW(...) \
  __android_log_print(ANDROID_LOG_WARN, "patchfield", __VA_ARGS__)

typedef struct {
  OPENSL_STREAM *os;
  int sample_rate;
  int buffer_frames;
  int shm_fd;
  void *shm_ptr;
  ptrdiff_t next_buffer;
} patchfield;

static void perform_cleanup(patchfield *pb) {
  int i, j, k;
  for (i = 0; i < MAX_MODULES; ++i) {
    audio_module *module = ami_get_audio_module(pb->shm_ptr, i);
    if (__sync_or_and_fetch(&module->status, 0) == 2) {
      int buffer_frames = (module->input_channels + module->output_channels) *
        pb->buffer_frames;
      pb->next_buffer -= buffer_frames;
      for (j = 0; j < MAX_MODULES; ++j) {
        audio_module *other = ami_get_audio_module(pb->shm_ptr, j);
        if (other->input_buffer > module->input_buffer) {
          other->input_buffer -= buffer_frames;
          other->output_buffer -= buffer_frames;
        }
        for (k = 0; k < MAX_CONNECTIONS; ++k) {
          connection *conn = other->input_connections + k;
          if (conn->source_index == i &&
              __sync_or_and_fetch(&conn->status, 0)) {
            int val = 1;
            while (val = __sync_val_compare_and_swap(&conn->status, val, 0));
          }
        }
      }
      __sync_bool_compare_and_swap(&module->status, 2, 0);
    } else {
      for (j = 0; j < MAX_CONNECTIONS; ++j) {
        connection *conn = module->input_connections + j;
        __sync_bool_compare_and_swap(&conn->status, 2, 0);
      }
    }
  }
}

static int is_running(patchfield *pb) {
  return opensl_is_running(pb->os);
}

static int add_module(patchfield *pb,
    int input_channels, int output_channels) {
  if (!is_running(pb)) {
    perform_cleanup(pb);
  }
  if ((pb->next_buffer + (input_channels + output_channels) *
        pb->buffer_frames) * sizeof(float) > smi_get_size()) {
    return -9;  // PatchfieldException.OUT_OF_BUFFER_SPACE
  }
  int i;
  for (i = 0; i < MAX_MODULES; ++i) {
    audio_module *module = ami_get_audio_module(pb->shm_ptr, i);
    if (__sync_or_and_fetch(&module->status, 0) == 0) {
      module->active = 0;
      module->in_use = 0;
      module->sample_rate = pb->sample_rate;
      module->buffer_frames = pb->buffer_frames;
      module->input_channels = input_channels;
      module->input_buffer = pb->next_buffer;
      pb->next_buffer += input_channels * pb->buffer_frames;
      module->output_channels = output_channels;
      module->output_buffer = pb->next_buffer;
      pb->next_buffer += output_channels * pb->buffer_frames;
      module->report =
        BARRIER_OFFSET * MEM_PAGE_SIZE / sizeof(simple_barrier_t) + i * 3;
      sb_clobber(ami_get_barrier(pb->shm_ptr, module->report));
      module->wake = module->report + 1;
      sb_clobber(ami_get_barrier(pb->shm_ptr, module->wake));
      module->ready = module->report + 2;
      sb_clobber(ami_get_barrier(pb->shm_ptr, module->ready));
      memset(module->input_connections, 0,
          MAX_CONNECTIONS * sizeof(connection));
      __sync_bool_compare_and_swap(&module->status, 0, 1);
      return i;
    }
  }
  return -5;  // PatchfieldException.TOO_MANY_MODULES
}

static int delete_module(patchfield *pb, int index) {
  audio_module *module = ami_get_audio_module(pb->shm_ptr, index);
  __sync_bool_compare_and_swap(&module->status, 1, 2);
  return 0;
}

static int activate_module(patchfield *pb, int index) {
  audio_module *module = ami_get_audio_module(pb->shm_ptr, index);
  __sync_bool_compare_and_swap(&module->active, 0, 1);
  return 0;
}
static int deactivate_module(patchfield *pb, int index) {
  audio_module *module = ami_get_audio_module(pb->shm_ptr, index);
  __sync_bool_compare_and_swap(&module->active, 1, 0);
  return 0;
}

static int is_active(patchfield *pb, int index) {
  audio_module *module = ami_get_audio_module(pb->shm_ptr, index);
  return __sync_or_and_fetch(&module->active, 0);
}

static int get_input_channels(patchfield *pb, int index) {
  audio_module *module = ami_get_audio_module(pb->shm_ptr, index);
  return module->input_channels;
}

static int get_output_channels(patchfield *pb, int index) {
  audio_module *module = ami_get_audio_module(pb->shm_ptr, index);
  return module->output_channels;
}

static int is_connected(patchfield *pb, int source_index, int source_port,
    int sink_index, int sink_port) {
  audio_module *sink = ami_get_audio_module(pb->shm_ptr, sink_index);
  int i;
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    connection *input = sink->input_connections + i;
    if (input->source_index == source_index &&
        input->source_port == source_port &&
        input->sink_port == sink_port &&
        __sync_or_and_fetch(&input->status, 0) == 1) {
      return 1;
    }
  }
  return 0;
}

static int connect_modules(patchfield *pb, int source_index, int source_port,
   int sink_index, int sink_port) {
  if (!is_running(pb)) {
    perform_cleanup(pb);
  }
  audio_module *sink = ami_get_audio_module(pb->shm_ptr, sink_index);
  int i;
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    connection *input = sink->input_connections + i;
    if (__sync_or_and_fetch(&input->status, 0) == 0) {
      input->sink_port = sink_port;
      input->source_index = source_index;
      input->source_port = source_port;
      __sync_bool_compare_and_swap(&input->status, 0, 1);
      return 0;
    }
  }
  return -7;  // PatchfieldException.TOO_MANY_CONNECTIONS
}

static int disconnect_modules(patchfield *pb, int source_index, int source_port,
   int sink_index, int sink_port) {
  audio_module *sink = ami_get_audio_module(pb->shm_ptr, sink_index);
  int i;
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    connection *input = sink->input_connections + i;
    if (input->source_index == source_index &&
        input->source_port == source_port &&
        input->sink_port == sink_port &&
        __sync_bool_compare_and_swap(&input->status, 1, 2)) {
      break;
    }
  }
  return 0;
}

static void release(patchfield *pb) {
  int i;
  opensl_close(pb->os);
  smi_unlock(pb->shm_ptr);
  smi_unmap(pb->shm_ptr);
  close(pb->shm_fd);
  free(pb);
}

#define ONE_BILLION 1000000000

static void add_nsecs(struct timespec *t, int dt) {
  t->tv_nsec += dt;
  if (t->tv_nsec >= ONE_BILLION) {
    ++t->tv_sec;
    t->tv_nsec -= ONE_BILLION;
  }
}

static const float float_to_short = SHRT_MAX;
static const float short_to_float = 1 / (1 + (float) SHRT_MAX);

static void process(void *context, int sample_rate, int buffer_frames,
     int input_channels, const short *input_buffer,
     int output_channels, short *output_buffer) {
  patchfield *pb = (patchfield *) context;
  struct timespec deadline;
  clock_gettime(CLOCK_MONOTONIC, &deadline);
  add_nsecs(&deadline, 100000);  // 0.1ms deadline for clients to report.
  int i, j;
  for (i = 0; i < MAX_MODULES; ++i) {
    audio_module *module = ami_get_audio_module(pb->shm_ptr, i);
    module->in_use =
      __sync_or_and_fetch(&module->status, 0) == 1 &&
      __sync_or_and_fetch(&module->active, 0) &&
      ((i < 2) || sb_wait_and_clear(
        ami_get_barrier(pb->shm_ptr, module->report), &deadline) == 0);
    if (module->in_use) {
      sb_clobber(ami_get_barrier(pb->shm_ptr, module->ready));
      for (j = 0; j < MAX_CONNECTIONS; ++j) {
        connection *conn = module->input_connections + j;
        conn->in_use = (__sync_or_and_fetch(&conn->status, 0) == 1);
      }
    }
  }
  audio_module *input = ami_get_audio_module(pb->shm_ptr, 0);
  if (input->in_use) {
    float *b = ami_get_audio_buffer(pb->shm_ptr, input->output_buffer);
    for (i = 0; i < input_channels; ++i) {
      for (j = 0; j < buffer_frames; ++j) {
        b[j] = input_buffer[i + j * input_channels] * short_to_float;
      }
      b += buffer_frames;
    }
    sb_wake(ami_get_barrier(pb->shm_ptr, input->ready));
  }
  int dt = (ONE_BILLION / sample_rate + 1) * buffer_frames;
  clock_gettime(CLOCK_MONOTONIC, &deadline);
  add_nsecs(&deadline, 2 * dt);  // Two-buffer-period processing deadline.
  for (i = 2; i < MAX_MODULES; ++i) {
    audio_module *module = ami_get_audio_module(pb->shm_ptr, i);
    if (module->in_use) {
      module->deadline.tv_sec = deadline.tv_sec;
      module->deadline.tv_nsec = deadline.tv_nsec;
      sb_wake(ami_get_barrier(pb->shm_ptr, module->wake));
    }
  }
  audio_module *output = ami_get_audio_module(pb->shm_ptr, 1);
  if (output->in_use) {
    ami_collect_input(pb->shm_ptr, 1);
    float *b = ami_get_audio_buffer(pb->shm_ptr, output->input_buffer);
    for (i = 0; i < output_channels; ++i) {
      for (j = 0; j < buffer_frames; ++j) {
        float v = b[j];
        output_buffer[i + j * output_channels] = (short) (float_to_short *
            (isnan(v) ? 0 : (v < -1.0f ? -1.0f : (v > 1.0f ? 1.0f : v))));
      }
      b += buffer_frames;
    }
  }
  for (i = 2; i < MAX_MODULES; ++i) {
    audio_module *module = ami_get_audio_module(pb->shm_ptr, i);
    if (module->in_use) {
      sb_wait(ami_get_barrier(pb->shm_ptr, module->ready),
          &module->deadline);
    }
  }
  perform_cleanup(pb);
}

static patchfield *create_instance(int sample_rate, int buffer_frames,
    int input_channels, int output_channels) {
  patchfield *pb = malloc(sizeof(patchfield));
  if (pb) {
    pb->sample_rate = sample_rate;
    pb->buffer_frames = buffer_frames;
    pb->next_buffer = BUFFER_OFFSET * MEM_PAGE_SIZE / sizeof(float);

    pb->shm_fd = smi_create();
    if (pb->shm_fd < 0) {
      LOGW("Unable to create shared memory.");
      free(pb);
      return NULL;
    }
    pb->shm_ptr = smi_map(pb->shm_fd);
    if (!pb->shm_ptr) {
      LOGW("Unable to map shared memory.");
      close(pb->shm_fd);
      free(pb);
      return NULL;
    }
    smi_lock(pb->shm_ptr);

    // Create OpenSL stream.
    pb->os = opensl_open(sample_rate,
        input_channels, output_channels, buffer_frames, process, pb);
    if (!pb->os) {
      smi_unlock(pb->shm_ptr);
      smi_unmap(pb->shm_ptr);
      close(pb->shm_fd);
      free(pb);
      return NULL;
    }

    int i;
    for (i = 0; i < MAX_MODULES; ++i) {
      audio_module *module = ami_get_audio_module(pb->shm_ptr, i);
      memset(module, 0, sizeof(audio_module));
    }
    activate_module(pb, add_module(pb, 0, input_channels));
    activate_module(pb, add_module(pb, output_channels, 0));
  }
  return pb;
}

JNIEXPORT jlong JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_createInstance
(JNIEnv *env, jobject obj, jint sample_rate, jint buffer_frames,
 int input_channels, int output_channels) {
  return (jlong) create_instance(sample_rate, buffer_frames,
      input_channels, output_channels);
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_releaseInstance
(JNIEnv *env, jobject obj, jlong p) {
  patchfield *pb = (patchfield *) p;
  release(pb);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_sendSharedMemoryFileDescriptor
(JNIEnv *env, jobject obj, jlong p) {
  patchfield *pb = (patchfield *) p;
  if (smi_send(pb->shm_fd) < 0) {
    LOGW("Failed to send file descriptor.");
    return -1;  // PatchfieldException.FAILURE
  }
  return 0;
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_start
(JNIEnv *env, jobject obj, jlong p) {
  patchfield *pb = (patchfield *) p;
  return opensl_start(pb->os);
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_stop
(JNIEnv *env, jobject obj, jlong p) {
  patchfield *pb = (patchfield *) p;
  opensl_pause(pb->os);
}

JNIEXPORT jboolean JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_isRunning
(JNIEnv *env, jobject obj, jlong p) {
  patchfield *pb = (patchfield *) p;
  return is_running(pb);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_createModule
(JNIEnv *env, jobject obj, jlong p, jint input_channels, jint output_channels) {
  patchfield *pb = (patchfield *) p;
  return add_module(pb, input_channels, output_channels);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_connectPorts
(JNIEnv *env, jobject obj, jlong p,
 jint source_index, jint source_port, jint sink_index, jint sink_port) {
  patchfield *pb = (patchfield *) p;
  return connect_modules(pb, source_index, source_port, sink_index, sink_port);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_disconnectPorts
(JNIEnv *env, jobject obj, jlong p,
 jint source_index, jint source_port, jint sink_index, jint sink_port) {
  patchfield *pb = (patchfield *) p;
  return disconnect_modules(pb, source_index, source_port, sink_index, sink_port);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_deleteModule
(JNIEnv *env, jobject obj, jlong p, jint index) {
  patchfield *pb = (patchfield *) p;
  return delete_module(pb, index);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_activateModule
(JNIEnv *env, jobject obj, jlong p, jint index) {
  patchfield *pb = (patchfield *) p;
  return activate_module(pb, index);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_deactivateModule
(JNIEnv *env, jobject obj, jlong p, jint index) {
  patchfield *pb = (patchfield *) p;
  return deactivate_module(pb, index);
}

JNIEXPORT jboolean JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_isActive
(JNIEnv *env, jobject obj, jlong p, jint index) {
  patchfield *pb = (patchfield *) p;
  return is_active(pb, index);
}

JNIEXPORT jboolean JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_isConnected
(JNIEnv *env, jobject obj, jlong p, jint sourceIndex, jint sourcePort,
 jint sinkIndex, jint sinkPort) {
  patchfield *pb = (patchfield *) p;
  return is_connected(pb, sourceIndex, sourcePort, sinkIndex, sinkPort);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_getInputChannels
(JNIEnv *env, jobject obj, jlong p, jint index) {
  patchfield *pb = (patchfield *) p;
  return get_input_channels(pb, index);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_getOutputChannels
(JNIEnv *env, jobject obj, jlong p, jint index) {
  patchfield *pb = (patchfield *) p;
  return get_output_channels(pb, index);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_getProtocolVersion
(JNIEnv *env, jobject obj, jlong p) {
  return PATCHFIELD_PROTOCOL_VERSION;
}
