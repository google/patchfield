#ifndef __AUDIO_MODULE_INTERNAL_H__
#define __AUDIO_MODULE_INTERNAL_H__

#include "audio_module.h"

#include <stddef.h>
#include <time.h>
#include <unistd.h>

#define MAX_MODULES 32 
#define MAX_CONNECTIONS 16 

#define MEM_PAGE_SIZE sysconf(_SC_PAGESIZE)
#define BARRIER_OFFSET (MAX_MODULES * sizeof(audio_module) / MEM_PAGE_SIZE + 1)
#define BUFFER_OFFSET \
  (BARRIER_OFFSET + MAX_MODULES * 3 * sizeof(int) / MEM_PAGE_SIZE + 1)

typedef struct {
  int status;  // 0: none; 1: current; 2: slated for deletion
  int in_use;

  int source_index;
  int source_port;
  int sink_port;
} connection;

typedef struct {
  int status;  // 0: none; 1: current; 2: slated for deletion
  int in_use;
  int active;

  int sample_rate;
  int buffer_frames;

  int input_channels;
  ptrdiff_t input_buffer;   // Storing buffers as offsets of type ptrdiff_t
  int output_channels;      // rather than pointers of type float* to render
  ptrdiff_t output_buffer;  // them independent of the shared memory location.

  connection input_connections[MAX_CONNECTIONS];

  struct timespec deadline;
  ptrdiff_t report;
  ptrdiff_t wake;
  ptrdiff_t ready;
} audio_module;

audio_module *ami_get_audio_module(void *p, int index);
float *ami_get_audio_buffer(void *p, ptrdiff_t offset);
int *ami_get_barrier(void *p, ptrdiff_t offset);
void ami_collect_input(void *p, int index);
size_t ami_get_protected_size();

#endif
