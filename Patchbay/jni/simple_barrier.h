/*
 * An atomic blocking boolean, designed to be robust when the underlying memory
 * location is overwritten by buggy or malicious code. Integer return values are
 * 0 for success, -1 for failure (due to timeouts), and -2 for errors (if the
 * memory location has been tampered with).
 */

#ifndef __SIMPLE_BARRIER_H__
#define __SIMPLE_BARRIER_H__

#include <time.h>

/*
 * Abstract data type representing a barrier.
 */
typedef int simple_barrier_t;

/*
 * If *p == 0, wait for another thread to invoke sb_wake(p). If *p == 1, return
 * immediately. The deadline is either NULL (i.e., no deadline) or an absolute
 * time measured with CLOCK_MONOTONIC.
 *
 * Use this function if multiple threads may be waiting on p.
 */
int sb_wait(simple_barrier_t *p, struct timespec *abstime);

/*
 * Like sb_wait, except it clears *p right after *p has become 1.
 *
 * Use this function if the current thread is the only thread waiting on p.
 */
int sb_wait_and_clear(simple_barrier_t *p, struct timespec *abstime);

/*
 * If *p == 0, set it to 1 and wake all threads waiting on p. If *p == 1, do
 * nothing.
 */
int sb_wake(simple_barrier_t *p);

/*
 * Clears *p, regardless of whether any threads are waiting on p.
 */
void sb_clobber(simple_barrier_t *p);

#endif
