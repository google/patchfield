#include "futex_barrier.h"

#include <limits.h>
#include <linux/futex.h>
#include <sys/linux-syscalls.h>
#include <unistd.h>

int fb_wait(int *p, struct timespec *deadline) {
  switch (__sync_or_and_fetch(p, 0)) {
    case 0:
      syscall(__NR_futex, p, FUTEX_WAIT, 0, NULL, NULL, 0);
      // __futex_wait(p, 0, deadline);
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

int fb_wait_and_clear(int *p, struct timespec *deadline) {
  switch (__sync_or_and_fetch(p, 0)) {
    case 0:
      syscall(__NR_futex, p, FUTEX_WAIT, 0, NULL, NULL, 0);
      // __futex_wait(p, 0, deadline);
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

int fb_wake(int *p) {
  if (__sync_bool_compare_and_swap(p, 0, 1)) {
    syscall(__NR_futex, p, FUTEX_WAKE, INT_MAX, NULL, NULL, 0);
    // __futex_wake(p, INT_MAX);
    return 0;
  } else {
    return -2;
  }
}

void fb_clobber(int *p) {
  int val = 1;
  while (val = __sync_val_compare_and_swap(p, val, 0));
}
