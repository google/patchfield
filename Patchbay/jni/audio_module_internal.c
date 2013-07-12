#include "audio_module_internal.h"

#include "simple_barrier.h"

#include <string.h>

audio_module *ami_get_audio_module(void *p, int index) {
  return ((audio_module *) p) + index;
}

float *ami_get_audio_buffer(void *p, ptrdiff_t offset) {
  return ((float *) p) + offset;
}

void ami_collect_input(void *p, int index) {
  audio_module *module = ami_get_audio_module(p, index);
  float *input_buffer = ami_get_audio_buffer(p, module->input_buffer);
  memset(input_buffer, 0,
      module->buffer_frames * module->input_channels * sizeof(float));
  int i, j;
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    connection *conn = module->input_connections + i;
    if (conn->in_use) {
      audio_module *source = ami_get_audio_module(p, conn->source_index);
      if (source->in_use) {
        float *input_channel =
          input_buffer + conn->sink_port * module->buffer_frames;
        float *source_channel = ami_get_audio_buffer(p,
            source->output_buffer) + conn->source_port * module->buffer_frames;
        if (!sb_wait(&source->ready, &source->deadline)) {
          for (j = 0; j < module->buffer_frames; ++j) {
            input_channel[j] += source_channel[j];
          }
        }
      }
    }
  }
}
