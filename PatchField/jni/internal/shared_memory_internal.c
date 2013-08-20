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

#include "shared_memory_internal.h"

#include <android/log.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/ashmem.h>

#define LOGI(...) \
  __android_log_print(ANDROID_LOG_INFO, "shared_memory_internal", __VA_ARGS__)
#define LOGW(...) \
  __android_log_print(ANDROID_LOG_WARN, "shared_memory_internal", __VA_ARGS__)

#define ASHMEM_MODULE "/dev/ashmem"
#define SHARED_MEM_SIZE 262144
#define SOCK_NAME "patchfield_shm_socket"

int smi_create() {
  int fd = open(ASHMEM_MODULE, O_RDWR);
  if (fd < 0) {
    LOGW("Failed to open ashmem: %s", strerror(errno));
    return -1;
  }
  if (ioctl(fd, ASHMEM_SET_SIZE, SHARED_MEM_SIZE) < 0) {
    LOGW("Failed to allocate shared memory: %s", strerror(errno));
    close(fd);
    return -1;
  }
  return fd;
}

void *smi_map(int fd) {
  return mmap(NULL, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

int smi_unmap(void *p) {
  return munmap(p, SHARED_MEM_SIZE);
}

int smi_lock(void *p) {
  int result = mlock(p, SHARED_MEM_SIZE);
  if (result) {
    LOGW("Failed to lock shared memory: %s", strerror(errno));
  } else {
    LOGI("Locked shared memory.");
  }
  return result;
}

int smi_unlock(void *p) {
  return munlock(p, SHARED_MEM_SIZE);
}

int smi_protect(void *p, size_t n) {
  mprotect(p, n, PROT_READ);
}

static int smi_transmit(int fd) {
  // Boilerplate for passing file descriptors across processes.
  int x = 1;
  struct iovec v;
  v.iov_base = &x;
  v.iov_len = sizeof(int);
  struct msghdr hdr;
  hdr.msg_name = NULL;
  hdr.msg_namelen = 0;
  hdr.msg_iov = &v;
  hdr.msg_iovlen = 1;
  union {
  struct cmsghdr chdr;
  char   control[CMSG_SPACE(sizeof(int))];
  } ctrl;
  hdr.msg_control = ctrl.control;
  hdr.msg_controllen = sizeof(ctrl.control);
  ctrl.chdr.cmsg_type = SCM_RIGHTS;
  ctrl.chdr.cmsg_level = SOL_SOCKET;
  ctrl.chdr.cmsg_len = CMSG_LEN(sizeof(int));

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  addr.sun_path[0] = 0;
  char *path = SOCK_NAME;
  strncpy(&addr.sun_path[1], path, strlen(path));
  int sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
  if (sock_fd < 0) {
    LOGW("Failed to open socket: %s", strerror(errno));
    return -1;
  }

  if (fd >= 0) {
    if (connect(sock_fd, (struct sockaddr *) &addr,
          sizeof(struct sockaddr_un)) < 0) {
      close(sock_fd);
      LOGW("Failed to connect socket: %s", strerror(errno));
      return -1;
    }
    *((int *) CMSG_DATA(CMSG_FIRSTHDR(&hdr))) = fd;
    if (sendmsg(sock_fd, &hdr, 0) < 0) {
      close(sock_fd);
      LOGW("Failed to pass file descriptor: %s", strerror(errno));
      return -1;
    }
    if (close(sock_fd) < 0) {
      LOGW("Failed to close socket: %s", strerror(errno));
    }
    return 0;
  } else {
    if (bind(sock_fd, (struct sockaddr *) &addr,
          sizeof(struct sockaddr_un)) < 0) {
      close(sock_fd);
      LOGW("Failed to bind socket: %s", strerror(errno));
      return -1;
    }
    if (recvmsg(sock_fd, &hdr, 0) < 0) {
      close(sock_fd);
      LOGW("Failed to receive file descriptor: %s", strerror(errno));
      return -1;
    }
    if (close(sock_fd) < 0) {
      LOGW("Failed to close socket: %s", strerror(errno));
    }
    return *((int *) CMSG_DATA(CMSG_FIRSTHDR(&hdr)));
  }
}

int smi_send(int fd) {
  if (fd < 0) {
    LOGW("Negative file descriptor.");
    return -1;
  }
  return smi_transmit(fd);
}

int smi_receive() {
  return smi_transmit(-1);
}

long smi_get_size() {
  return SHARED_MEM_SIZE;
}
