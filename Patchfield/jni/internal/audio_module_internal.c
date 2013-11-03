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

#include "audio_module_internal.h"

#include "opensl_stream/opensl_stream.h"
#include "audio_module_internal.h"
#include "shared_memory_internal.h"
#include "simple_barrier.h"

#include <android/log.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#define LOGI(...) \
  __android_log_print(ANDROID_LOG_INFO, "audio_module_internal", __VA_ARGS__)
#define LOGW(...) \
  __android_log_print(ANDROID_LOG_WARN, "audio_module_internal", __VA_ARGS__)

audio_module *ami_get_audio_module(void *p, int index) {
  return ((audio_module *) p) + index;
}

float *ami_get_audio_buffer(void *p, ptrdiff_t offset) {
  return ((float *) p) + offset;
}

simple_barrier_t *ami_get_barrier(void *p, ptrdiff_t offset) {
  return ((simple_barrier_t *) p) + offset;
}

void ami_collect_input(void *p, int index) {
  audio_module *module = ami_get_audio_module(p, index);
  float *input_buffer = ami_get_audio_buffer(p, module->input_buffer);
  memset(input_buffer, 0,
      module->buffer_frames * module->input_channels * sizeof(float));
  int i, j;
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    connection *conn = module->input_connections + i;
    if (conn->in_use) {
      audio_module *source = ami_get_audio_module(p, conn->source_index);
      if (source->in_use) {
        float *input_channel =
          input_buffer + conn->sink_port * module->buffer_frames;
        float *source_channel = ami_get_audio_buffer(p,
            source->output_buffer) + conn->source_port * module->buffer_frames;
        if (!sb_wait(ami_get_barrier(p, source->ready), &source->deadline)) {
          for (j = 0; j < module->buffer_frames; ++j) {
            input_channel[j] += source_channel[j];
          }
        }
      }
    }
  }
}

#define AM_SIG_ALRM SIGRTMAX

static __thread sigjmp_buf sig_env;

static void signal_handler(int sig, siginfo_t *info, void *context) {
  LOGI("Received signal %d.", sig);
  siglongjmp(sig_env, 1);
}

static void *run_module(void *arg) {
  LOGI("Entering run_module.");
  audio_module_runner *amr = (audio_module_runner *) arg;
  sb_wake(&amr->launched);
  audio_module *module = ami_get_audio_module(amr->shm_ptr, amr->index);

  timer_t timer;
  struct sigevent evp;
  evp.sigev_notify = SIGEV_THREAD_ID;
  evp.sigev_signo = AM_SIG_ALRM;
  evp.sigev_value.sival_ptr = module;
  evp.sigev_notify_thread_id = gettid();
  timer_create(CLOCK_MONOTONIC, &evp, &timer);

  struct itimerspec timeout;
  timeout.it_interval.tv_sec = 0;
  timeout.it_interval.tv_nsec = 0;
  timeout.it_value.tv_sec = 1;  // One-second timeout.
  timeout.it_value.tv_nsec = 0;

  struct itimerspec cancel;
  cancel.it_interval.tv_sec = 0;
  cancel.it_interval.tv_nsec = 0;
  cancel.it_value.tv_sec = 0;
  cancel.it_value.tv_nsec = 0;

  if (!sigsetjmp(sig_env, 1)) {
    while (1) {
      sb_wake(ami_get_barrier(amr->shm_ptr, module->report));
      sb_wait_and_clear(ami_get_barrier(amr->shm_ptr, module->wake), NULL);
      if (amr->done) {
        break;
      }
      ami_collect_input(amr->shm_ptr, amr->index);
      timer_settime(timer, 0, &timeout, NULL);  // Arm timer.
      amr->process(amr->context, module->sample_rate, module->buffer_frames,
          module->input_channels,
          ami_get_audio_buffer(amr->shm_ptr, module->input_buffer),
          module->output_channels,
          ami_get_audio_buffer(amr->shm_ptr, module->output_buffer));
      timer_settime(timer, 0, &cancel, NULL);  // Disarm timer.
      sb_wake(ami_get_barrier(amr->shm_ptr, module->ready));
    }
  } else {
    __sync_bool_compare_and_swap(&amr->timed_out, 0, 1);
    // We can safely log now because we have already left the processing chain.
    LOGW("Process callback interrupted after timeout; terminating thread.");
  }

  timer_delete(timer);
  LOGI("Leaving run_module.");
  return NULL;
}

static void launch_thread(void *context, int sample_rate, int buffer_frames,
    int input_channels, const short *input_buffer,
    int output_channels, short *output_buffer) {
  audio_module_runner *amr = (audio_module_runner *) context;
  if (!--amr->launch_counter) {
    if (!pthread_create(&amr->thread, NULL, run_module, amr)) {
      pthread_setname_np(amr->thread, "AudioModule");
    } else {
      LOGW("Thread creation failed: %s", strerror(errno));
    }
  }
}

static size_t get_protected_size() {
  return BARRIER_OFFSET * MEM_PAGE_SIZE;
}

audio_module_runner *ami_create(int version, int token, int index) {
  if (version != PATCHFIELD_PROTOCOL_VERSION) {
    LOGW("Protocol version mismatch.");
    return NULL;
  }
  audio_module_runner *amr = malloc(sizeof(audio_module_runner));
  if (amr) {
    amr->shm_fd = token;
    amr->shm_ptr = smi_map(token);
    smi_protect(amr->shm_ptr, get_protected_size());
    amr->index = index;
    amr->done = 0;
    amr->timed_out = 0;
    amr->process = NULL;
    amr->context = NULL;
    amr->launch_counter = 3;  // Make sure that this number stays current.

    audio_module *module = ami_get_audio_module(amr->shm_ptr, amr->index);
    // Clear barriers, just in case.
    sb_clobber(ami_get_barrier(amr->shm_ptr, module->report));
    sb_clobber(ami_get_barrier(amr->shm_ptr, module->wake));
    sb_clobber(ami_get_barrier(amr->shm_ptr, module->ready));

    OPENSL_STREAM *os = opensl_open(module->sample_rate, 0, 2,
        module->buffer_frames, launch_thread, amr);
    sb_clobber(&amr->launched);
    opensl_start(os);
    sb_wait(&amr->launched, NULL);
    opensl_close(os);

    struct sigaction act;
    act.sa_sigaction = signal_handler;
    act.sa_flags = SA_SIGINFO;
    sigfillset(&act.sa_mask);
    sigaction(AM_SIG_ALRM, &act, NULL);
  }
  return (audio_module_runner *) amr;
}

void ami_release(audio_module_runner *amr) {
  audio_module *module = ami_get_audio_module(amr->shm_ptr, amr->index);

  amr->done = 1;
  sb_wake(ami_get_barrier(amr->shm_ptr, module->wake));
  pthread_join(amr->thread, NULL);

  smi_unmap(amr->shm_ptr);
  free(amr);
}

int ami_has_timed_out(audio_module_runner *amr) {
  return __sync_or_and_fetch(&amr->timed_out, 0);
}
