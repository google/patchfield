#include "audio_module.h"

#include "audio_module_internal.h"
#include "simple_barrier.h"
#include "opensl_stream/opensl_stream.h"
#include "shared_memory_internal.h"

#include <android/log.h>
#include <errno.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#define LOGI(...) \
  __android_log_print(ANDROID_LOG_INFO, "audio_module", __VA_ARGS__)
#define LOGW(...) \
  __android_log_print(ANDROID_LOG_WARN, "audio_module", __VA_ARGS__)

struct _audio_module_runner {
  int shm_fd;
  void *shm_ptr;
  int index;
  pthread_t thread;
  int launched;
  int launch_counter;
  int done;
  int timed_out;
  audio_module_process_t process;
  void *context;
};

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

  while (1) {
    sb_wake(ami_get_barrier(amr->shm_ptr, module->report));
    sb_wait_and_clear(ami_get_barrier(amr->shm_ptr, module->wake), NULL);
    if (amr->done) {
      break;
    }
    ami_collect_input(amr->shm_ptr, amr->index);
    if (!sigsetjmp(sig_env, 1)) {
      struct itimerspec timerspec;
      timerspec.it_interval.tv_sec = 0;
      timerspec.it_interval.tv_nsec = 0;
      timerspec.it_value.tv_sec = 1;  // One-second timeout.
      timerspec.it_value.tv_nsec = 0;
      timer_settime(timer, 0, &timerspec, NULL);  // Arm timer.
      amr->process(amr->context, module->sample_rate, module->buffer_frames,
          module->input_channels,
          ami_get_audio_buffer(amr->shm_ptr, module->input_buffer),
          module->output_channels,
          ami_get_audio_buffer(amr->shm_ptr, module->output_buffer));
      timerspec.it_value.tv_sec = 0;
      timer_settime(timer, 0, &timerspec, NULL);  // Disarm timer.
    } else {
      __sync_bool_compare_and_swap(&amr->timed_out, 0, 1);
      // We can safely log now because we are leaving the processing chain.
      LOGW("Process callback interrupted after timeout; terminating thread.");
      break;
    }
    sb_wake(ami_get_barrier(amr->shm_ptr, module->ready));
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
    if (pthread_create(&amr->thread, NULL, run_module, amr)) {
      LOGW("Thread creation failed: %s", strerror(errno));
    }
  }
}

audio_module_runner *am_create(int version, int token, int index,
    audio_module_process_t process, void *context) {
  if (version != PATCHBAY_PROTOCOL_VERSION) {
    LOGW("Protocol version mismatch.");
    return NULL;
  }
  audio_module_runner *amr = malloc(sizeof(audio_module_runner));
  if (amr) {
    amr->shm_fd = token;
    amr->shm_ptr = smi_map(token);
    smi_protect(amr->shm_ptr, ami_get_protected_size());
    amr->index = index;
    amr->done = 0;
    amr->timed_out = 0;
    amr->process = process;
    amr->context = (void *) context;
    amr->launch_counter = 3;  // Make sure that this number stays current.

    audio_module *module = ami_get_audio_module(amr->shm_ptr, amr->index);
    // Clear barriers, just in case.
    sb_clobber(ami_get_barrier(amr->shm_ptr, module->report));
    sb_clobber(ami_get_barrier(amr->shm_ptr, module->wake));
    sb_clobber(ami_get_barrier(amr->shm_ptr, module->ready));

    OPENSL_STREAM *os = opensl_open(module->sample_rate, 0, 2,
        module->buffer_frames, launch_thread, amr);
    amr->launched = 0;
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

void am_release(audio_module_runner *amr) {
  audio_module *module = ami_get_audio_module(amr->shm_ptr, amr->index);

  amr->done = 1;
  sb_wake(ami_get_barrier(amr->shm_ptr, module->wake));
  pthread_join(amr->thread, NULL);

  smi_unmap(amr->shm_ptr);
  free(amr);
}

int am_has_timed_out(audio_module_runner *amr) {
  return __sync_or_and_fetch(&amr->timed_out, 0);
}

int am_get_sample_rate(audio_module_runner *amr) {
  audio_module *module = ami_get_audio_module(amr->shm_ptr, amr->index);
  return module->sample_rate;
}

int am_get_buffer_frames(audio_module_runner *amr) {
  audio_module *module = ami_get_audio_module(amr->shm_ptr, amr->index);
  return module->buffer_frames;
}
