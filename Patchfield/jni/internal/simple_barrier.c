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

#include "simple_barrier.h"

#include <limits.h>
#include <linux/futex.h>
#include <sys/linux-syscalls.h>
#include <unistd.h>

#define ONE_BILLION 1000000000

static void get_relative_deadline(
    const struct timespec *abstime, struct timespec *reltime) {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  if (now.tv_sec < abstime->tv_sec ||
      (now.tv_sec == abstime->tv_sec && now.tv_nsec < abstime->tv_nsec)) {
    reltime->tv_sec = abstime->tv_sec - now.tv_sec;
    if (abstime->tv_nsec >= now.tv_nsec) {
      reltime->tv_nsec = abstime->tv_nsec - now.tv_nsec;
    } else {
      --reltime->tv_sec;
      reltime->tv_nsec = (ONE_BILLION + abstime->tv_nsec) - now.tv_nsec;
    }
  } else {
    reltime->tv_sec = 0;
    reltime->tv_nsec = 0;
  }
}

static void futex_wait(simple_barrier_t *p, struct timespec *abstime) {
  if (abstime == NULL) {
    syscall(__NR_futex, p, FUTEX_WAIT, 0, NULL, NULL, 0, 0);
  } else {
    struct timespec reltime;
    get_relative_deadline(abstime, &reltime);
    syscall(__NR_futex, p, FUTEX_WAIT, 0, &reltime, NULL, 0, 0);
    // Note: Passing struct timespec as ktime_t works for now but may need
    // further consideration when we move to 64bit.
  }
}

int sb_wait(simple_barrier_t *p, struct timespec *abstime) {
  switch (__sync_or_and_fetch(p, 0)) {
    case 0:
      futex_wait(p, abstime);
      break;
    case 1:
      return 0;   // Success!
    default:
      return -2;  // Error; the futex has been tampered with.
  }
  switch (__sync_or_and_fetch(p, 0)) {
    case 0:
      return -1;  // Failure; __futex_wait probably timed out.
    case 1:
      return 0;
    default:
      return -2;
  }
}

int sb_wait_and_clear(simple_barrier_t *p, struct timespec *abstime) {
  switch (__sync_or_and_fetch(p, 0)) {
    case 0:
      futex_wait(p, abstime);
    case 1:
      break;
    default:
      return -2;
  }
  switch (__sync_val_compare_and_swap(p, 1, 0)) {
    case 0:
      return -1;
    case 1:
      return 0;
    default:
      return -2;
  }
}

int sb_wake(simple_barrier_t *p) {
  if (__sync_bool_compare_and_swap(p, 0, 1)) {
    syscall(__NR_futex, p, FUTEX_WAKE, INT_MAX, NULL, NULL, 0, 0);
    return 0;
  } else {
    return -2;
  }
}

void sb_clobber(simple_barrier_t *p) {
  int val = 1;
  while (val = __sync_val_compare_and_swap(p, val, 0));
}
