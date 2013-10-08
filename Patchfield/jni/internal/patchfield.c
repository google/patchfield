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
  ptrdiff_t msg_read_ptr;
  ptrdiff_t msg_write_ptr;
} patchfield;

static void perform_cleanup(patchfield *pf) {
  int i, j, k;
  for (i = 0; i < MAX_MODULES; ++i) {
    audio_module *module = ami_get_audio_module(pf->shm_ptr, i);
    if (__sync_or_and_fetch(&module->status, 0) == 2) {
      int buffer_frames = (module->input_channels + module->output_channels) *
        pf->buffer_frames;
      pf->next_buffer -= buffer_frames;
      for (j = 0; j < MAX_MODULES; ++j) {
        audio_module *other = ami_get_audio_module(pf->shm_ptr, j);
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

static int is_running(patchfield *pf) {
  return opensl_is_running(pf->os);
}

static int add_module(patchfield *pf,
    int input_channels, int output_channels) {
  if (!is_running(pf)) {
    perform_cleanup(pf);
  }
  if ((pf->next_buffer + (input_channels + output_channels) *
        pf->buffer_frames) * sizeof(float) > smi_get_size()) {
    return -9;  // PatchfieldException.OUT_OF_BUFFER_SPACE
  }
  int i;
  for (i = 0; i < MAX_MODULES; ++i) {
    audio_module *module = ami_get_audio_module(pf->shm_ptr, i);
    if (__sync_or_and_fetch(&module->status, 0) == 0) {
      module->active = 0;
      module->in_use = 0;
      module->sample_rate = pf->sample_rate;
      module->buffer_frames = pf->buffer_frames;
      module->input_channels = input_channels;
      module->input_buffer = pf->next_buffer;
      pf->next_buffer += input_channels * pf->buffer_frames;
      module->output_channels = output_channels;
      module->output_buffer = pf->next_buffer;
      pf->next_buffer += output_channels * pf->buffer_frames;
      module->report =
        BARRIER_OFFSET * MEM_PAGE_SIZE / sizeof(simple_barrier_t) + i * 3;
      sb_clobber(ami_get_barrier(pf->shm_ptr, module->report));
      module->wake = module->report + 1;
      sb_clobber(ami_get_barrier(pf->shm_ptr, module->wake));
      module->ready = module->report + 2;
      sb_clobber(ami_get_barrier(pf->shm_ptr, module->ready));
      memset(module->input_connections, 0,
          MAX_CONNECTIONS * sizeof(connection));
      __sync_bool_compare_and_swap(&module->status, 0, 1);
      return i;
    }
  }
  return -5;  // PatchfieldException.TOO_MANY_MODULES
}

static int delete_module(patchfield *pf, int index) {
  audio_module *module = ami_get_audio_module(pf->shm_ptr, index);
  __sync_bool_compare_and_swap(&module->status, 1, 2);
  return 0;
}

static int activate_module(patchfield *pf, int index) {
  audio_module *module = ami_get_audio_module(pf->shm_ptr, index);
  __sync_bool_compare_and_swap(&module->active, 0, 1);
  return 0;
}
static int deactivate_module(patchfield *pf, int index) {
  audio_module *module = ami_get_audio_module(pf->shm_ptr, index);
  __sync_bool_compare_and_swap(&module->active, 1, 0);
  return 0;
}

static int is_active(patchfield *pf, int index) {
  audio_module *module = ami_get_audio_module(pf->shm_ptr, index);
  return __sync_or_and_fetch(&module->active, 0);
}

static int get_input_channels(patchfield *pf, int index) {
  audio_module *module = ami_get_audio_module(pf->shm_ptr, index);
  return module->input_channels;
}

static int get_output_channels(patchfield *pf, int index) {
  audio_module *module = ami_get_audio_module(pf->shm_ptr, index);
  return module->output_channels;
}

static int is_connected(patchfield *pf, int source_index, int source_port,
    int sink_index, int sink_port) {
  audio_module *sink = ami_get_audio_module(pf->shm_ptr, sink_index);
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

static int connect_modules(patchfield *pf, int source_index, int source_port,
   int sink_index, int sink_port) {
  if (!is_running(pf)) {
    perform_cleanup(pf);
  }
  audio_module *sink = ami_get_audio_module(pf->shm_ptr, sink_index);
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

static int disconnect_modules(patchfield *pf, int source_index, int source_port,
   int sink_index, int sink_port) {
  audio_module *sink = ami_get_audio_module(pf->shm_ptr, sink_index);
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

static void release(patchfield *pf) {
  int i;
  opensl_close(pf->os);
  smi_unlock(pf->shm_ptr);
  smi_unmap(pf->shm_ptr);
  close(pf->shm_fd);
  free(pf);
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
  patchfield *pf = (patchfield *) context;
  ptrdiff_t read_ptr = __sync_fetch_and_or(&pf->msg_read_ptr, 0);
  ptrdiff_t write_ptr = __sync_fetch_and_or(&pf->msg_write_ptr, 0);
  *(ptrdiff_t *)ami_get_message_buffer(pf->shm_ptr,
      ami_get_read_ptr_offset) = read_ptr;
  *(ptrdiff_t *)ami_get_message_buffer(pf->shm_ptr,
      ami_get_write_ptr_offset) = write_ptr;
  struct timespec deadline;
  clock_gettime(CLOCK_MONOTONIC, &deadline);
  add_nsecs(&deadline, 100000);  // 0.1ms deadline for clients to report.
  int i, j;
  for (i = 0; i < MAX_MODULES; ++i) {
    audio_module *module = ami_get_audio_module(pf->shm_ptr, i);
    module->in_use =
      __sync_or_and_fetch(&module->status, 0) == 1 &&
      __sync_or_and_fetch(&module->active, 0) &&
      ((i < 2) || sb_wait_and_clear(
        ami_get_barrier(pf->shm_ptr, module->report), &deadline) == 0);
    if (module->in_use) {
      sb_clobber(ami_get_barrier(pf->shm_ptr, module->ready));
      for (j = 0; j < MAX_CONNECTIONS; ++j) {
        connection *conn = module->input_connections + j;
        conn->in_use = (__sync_or_and_fetch(&conn->status, 0) == 1);
      }
    }
  }
  audio_module *input = ami_get_audio_module(pf->shm_ptr, 0);
  if (input->in_use) {
    float *b = ami_get_audio_buffer(pf->shm_ptr, input->output_buffer);
    for (i = 0; i < input_channels; ++i) {
      for (j = 0; j < buffer_frames; ++j) {
        b[j] = input_buffer[i + j * input_channels] * short_to_float;
      }
      b += buffer_frames;
    }
    sb_wake(ami_get_barrier(pf->shm_ptr, input->ready));
  }
  int dt = (ONE_BILLION / sample_rate + 1) * buffer_frames;
  clock_gettime(CLOCK_MONOTONIC, &deadline);
  add_nsecs(&deadline, 2 * dt);  // Two-buffer-period processing deadline.
  for (i = 2; i < MAX_MODULES; ++i) {
    audio_module *module = ami_get_audio_module(pf->shm_ptr, i);
    if (module->in_use) {
      module->deadline.tv_sec = deadline.tv_sec;
      module->deadline.tv_nsec = deadline.tv_nsec;
      sb_wake(ami_get_barrier(pf->shm_ptr, module->wake));
    }
  }
  audio_module *output = ami_get_audio_module(pf->shm_ptr, 1);
  if (output->in_use) {
    ami_collect_input(pf->shm_ptr, 1);
    float *b = ami_get_audio_buffer(pf->shm_ptr, output->input_buffer);
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
    audio_module *module = ami_get_audio_module(pf->shm_ptr, i);
    if (module->in_use) {
      sb_wait(ami_get_barrier(pf->shm_ptr, module->ready),
          &module->deadline);
    }
  }
  perform_cleanup(pf);
  __sync_bool_compare_and_swap(&pf->msg_read_ptr, read_ptr, write_ptr);
}

static patchfield *create_instance(int sample_rate, int buffer_frames,
    int input_channels, int output_channels) {
  patchfield *pf = malloc(sizeof(patchfield));
  if (pf) {
    pf->sample_rate = sample_rate;
    pf->buffer_frames = buffer_frames;
    pf->next_buffer = BUFFER_OFFSET * MEM_PAGE_SIZE / sizeof(float);
    pf->msg_read_ptr = ami_get_data_offset;
    pf->msg_write_ptr = ami_get_data_offset;
    pf->shm_fd = smi_create();
    if (pf->shm_fd < 0) {
      LOGW("Unable to create shared memory.");
      free(pf);
      return NULL;
    }
    pf->shm_ptr = smi_map(pf->shm_fd);
    if (!pf->shm_ptr) {
      LOGW("Unable to map shared memory.");
      close(pf->shm_fd);
      free(pf);
      return NULL;
    }
    smi_lock(pf->shm_ptr);

    // Create OpenSL stream.
    pf->os = opensl_open(sample_rate,
        input_channels, output_channels, buffer_frames, process, pf);
    if (!pf->os) {
      smi_unlock(pf->shm_ptr);
      smi_unmap(pf->shm_ptr);
      close(pf->shm_fd);
      free(pf);
      return NULL;
    }

    int i;
    for (i = 0; i < MAX_MODULES; ++i) {
      audio_module *module = ami_get_audio_module(pf->shm_ptr, i);
      memset(module, 0, sizeof(audio_module));
    }
    activate_module(pf, add_module(pf, 0, input_channels));
    activate_module(pf, add_module(pf, output_channels, 0));
  }
  return pf;
}

static int post_message(patchfield *pf, int32_t length, const char *data) {
  if (length > MAX_MESSAGE_LENGTH) {
    return -12;  // PatchfieldException.MESSAGE_TOO_LONG
  }
  if (length == 0) {
    return -13;  // PatchfieldException.EMPTY_MESSAGE
  }
  if (length < 0) {
    return -2;  // PatchfieldException.INVALID_PARAMETERS
  }
  ptrdiff_t rp = __sync_fetch_and_or(&pf->msg_read_ptr, 0);
  ptrdiff_t wp = __sync_fetch_and_or(&pf->msg_write_ptr, 0);
  ptrdiff_t wn;
  if (rp > wp) {
    if (rp - wp > length + 4) {
      wn = wp;
    } else {
      LOGW("Message queue full.");
      return -11;  // PatchfieldException.INSUFFICIENT_MESSAGE_SPACE
    }
  } else if (ami_get_top_offset - wp > length + 4) {
    wn = wp;
  } else if (rp - ami_get_data_offset > length + 4) {
    wn = ami_get_data_offset;
  } else {
    LOGW("Message queue full.");
    return -11;  // PatchfieldException.INSUFFICIENT_MESSAGE_SPACE
  }
  char *p = ami_get_message_buffer(pf->shm_ptr, wn);
  *(int32_t *)p = length;
  memcpy(p + 4, data, length);
  if (length & 0x03) {
    length += 4 - (length & 0x03);
  }
  *(int32_t *)(p + length + 4) = 0;
  __sync_bool_compare_and_swap(&pf->msg_write_ptr, wp, wn + length + 4);
  return 0;
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
  patchfield *pf = (patchfield *) p;
  release(pf);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_sendSharedMemoryFileDescriptor
(JNIEnv *env, jobject obj, jlong p) {
  patchfield *pf = (patchfield *) p;
  if (smi_send(pf->shm_fd) < 0) {
    LOGW("Failed to send file descriptor.");
    return -1;  // PatchfieldException.FAILURE
  }
  return 0;
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_start
(JNIEnv *env, jobject obj, jlong p) {
  patchfield *pf = (patchfield *) p;
  return opensl_start(pf->os);
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_stop
(JNIEnv *env, jobject obj, jlong p) {
  patchfield *pf = (patchfield *) p;
  opensl_pause(pf->os);
}

JNIEXPORT jboolean JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_isRunning
(JNIEnv *env, jobject obj, jlong p) {
  patchfield *pf = (patchfield *) p;
  return is_running(pf);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_createModule
(JNIEnv *env, jobject obj, jlong p, jint input_channels, jint output_channels) {
  patchfield *pf = (patchfield *) p;
  return add_module(pf, input_channels, output_channels);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_connectPorts
(JNIEnv *env, jobject obj, jlong p,
 jint source_index, jint source_port, jint sink_index, jint sink_port) {
  patchfield *pf = (patchfield *) p;
  return connect_modules(pf, source_index, source_port, sink_index, sink_port);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_disconnectPorts
(JNIEnv *env, jobject obj, jlong p,
 jint source_index, jint source_port, jint sink_index, jint sink_port) {
  patchfield *pf = (patchfield *) p;
  return disconnect_modules(pf, source_index, source_port, sink_index, sink_port);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_deleteModule
(JNIEnv *env, jobject obj, jlong p, jint index) {
  patchfield *pf = (patchfield *) p;
  return delete_module(pf, index);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_activateModule
(JNIEnv *env, jobject obj, jlong p, jint index) {
  patchfield *pf = (patchfield *) p;
  return activate_module(pf, index);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_deactivateModule
(JNIEnv *env, jobject obj, jlong p, jint index) {
  patchfield *pf = (patchfield *) p;
  return deactivate_module(pf, index);
}

JNIEXPORT jboolean JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_isActive
(JNIEnv *env, jobject obj, jlong p, jint index) {
  patchfield *pf = (patchfield *) p;
  return is_active(pf, index);
}

JNIEXPORT jboolean JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_isConnected
(JNIEnv *env, jobject obj, jlong p, jint sourceIndex, jint sourcePort,
 jint sinkIndex, jint sinkPort) {
  patchfield *pf = (patchfield *) p;
  return is_connected(pf, sourceIndex, sourcePort, sinkIndex, sinkPort);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_getInputChannels
(JNIEnv *env, jobject obj, jlong p, jint index) {
  patchfield *pf = (patchfield *) p;
  return get_input_channels(pf, index);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_getOutputChannels
(JNIEnv *env, jobject obj, jlong p, jint index) {
  patchfield *pf = (patchfield *) p;
  return get_output_channels(pf, index);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_getProtocolVersion
(JNIEnv *env, jobject obj, jlong p) {
  return PATCHFIELD_PROTOCOL_VERSION;
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_getMaxMessageLength
(JNIEnv *env, jobject obj, jlong p) {
  return MAX_MESSAGE_LENGTH;
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_Patchfield_postMessage
(JNIEnv *env, jobject obj, jlong p, jint length, jbyteArray data) {
  patchfield *pf = (patchfield *) p;
  int n = (*env)->GetArrayLength(env, data);
  char *b = (*env)->GetByteArrayElements(env, data, NULL);
  int res = post_message(pf, length, b);
  (*env)->ReleaseByteArrayElements(env, data, b, 0);
  return res;
}

