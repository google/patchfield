#ifndef __FUTEX_BARRIER_H__
#define __FUTEX_BARRIER_H__

#include <time.h>

int sb_wait(int *p, struct timespec *abstime);
int sb_wait_and_clear(int *p, struct timespec *abstime);
int sb_wake(int *p);
void sb_clobber(int *p);

#endif
