#ifndef __SHARED_MEMORY_INTERNAL_H__
#define __SHARED_MEMORY_INTERNAL_H__

int smi_create();
void *smi_map(int fd);
int smi_unmap(void *p);
int smi_lock(void *p);
int smi_unlock(void *p);
int smi_send(int fd);
int smi_receive();
long smi_get_size();

#endif
