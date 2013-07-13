/*
 * Tools for handling shared memory with ashmem. Most functions simply wrap basic
 * system calls, except for smi_{send,receive}, which serve to pass the ashmem
 * file descriptor across process boundaries using Unix domain sockets.
 */

#ifndef __SHARED_MEMORY_INTERNAL_H__
#define __SHARED_MEMORY_INTERNAL_H__

#include <stddef.h>

int smi_create();
void *smi_map(int fd);
int smi_unmap(void *p);
int smi_lock(void *p);
int smi_unlock(void *p);
int smi_protect(void *p, size_t n);
int smi_send(int fd);
int smi_receive();
long smi_get_size();

#endif
