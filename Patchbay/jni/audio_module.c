#include "audio_module.h"

#include "opensl_stream/opensl_stream.h"
#include "audio_module_internal.h"
#include "shared_memory_internal.h"

#include <android/log.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/atomics.h>
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
  sem_t launched;
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
  sem_post(&amr->launched);
  audio_module *module = ami_get_audio_module(amr->shm_ptr, amr->index);

  timer_t timer;
  struct sigevent evp;
  evp.sigev_notify = SIGEV_THREAD_ID;
  evp.sigev_signo = AM_SIG_ALRM;
  evp.sigev_value.sival_ptr = module;
  evp.sigev_notify_thread_id = gettid();
  timer_create(CLOCK_MONOTONIC, &evp, &timer);

  while (1) {
    if (__sync_bool_compare_and_swap(&module->report, 0, 1)) {
      __futex_wake(&module->report, INT_MAX);
    } else {
      while (!__sync_bool_compare_and_swap(&module->report, module->report, 0));
    }
    sem_wait(&module->wake);
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
      ami_notify_dependents(amr->shm_ptr, amr->index);
    } else {
      __sync_bool_compare_and_swap(&amr->timed_out, 0, 1);
      // We can safely log now because we are leaving the processing chain.
      LOGW("Process callback interrupted after timeout; terminating thread.");
      break;
    }
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

audio_module_runner *am_create(int token, int index,
    audio_module_process_t process, void *context) {
  audio_module_runner *amr = malloc(sizeof(audio_module_runner));
  if (amr) {
    amr->shm_fd = token;
    amr->shm_ptr = smi_map(token);
    amr->index = index;
    amr->done = 0;
    amr->timed_out = 0;
    amr->process = process;
    amr->context = (void *) context;
    amr->launch_counter = 3;  // Make sure that this number stays current.

    audio_module *module = ami_get_audio_module(amr->shm_ptr, amr->index);
    // Clear semaphores, just in case.
    while (!__sync_bool_compare_and_swap(&module->report, module->report, 0));
    while (!sem_trywait(&module->wake));
    while (!sem_trywait(&module->ready));

    OPENSL_STREAM *os = opensl_open(module->sample_rate, 0, 2,
        module->buffer_frames, launch_thread, amr);
    sem_init(&amr->launched, 0, 0);
    opensl_start(os);
    sem_wait(&amr->launched);
    opensl_close(os);
    sem_destroy(&amr->launched);

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
  sem_post(&module->wake);
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
