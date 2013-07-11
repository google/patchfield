#ifndef __FUTEX_BARRIER_H__
#define __FUTEX_BARRIER_H__

#include <time.h>

int fb_wait(int *p, struct timespec *abstime);
int fb_wait_and_clear(int *p, struct timespec *abstime);
int fb_wake(int *p);
void fb_clobber(int *p);

#endif
