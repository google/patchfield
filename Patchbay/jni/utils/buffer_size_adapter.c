#include "buffer_size_adapter.h"

#include <stdlib.h>

typedef struct {
  int buffer_frames;
  float *v;
  int write_index;
  int read_index;
} bsa_ring_buffer;

struct _buffer_size_adapter {
  int host_buffer_frames;
  int user_buffer_frames;
  void *user_context;
  audio_module_process_t user_process;
  bsa_ring_buffer *input_buffer;
  bsa_ring_buffer *output_buffer;
};

static int lcm(int a, int b) {
  int m = a;
  while (m % b) {
    m += a;
  }
  return m;
}

static int frames_available(bsa_ring_buffer *rb) {
  return
    (rb->buffer_frames + rb->write_index - rb->read_index) % rb->buffer_frames;
}

static bsa_ring_buffer *create_buffer(
    int host_buffer_frames, int user_buffer_frames, int channels) {
  bsa_ring_buffer *rb = malloc(sizeof(bsa_ring_buffer));
  if (rb) {
    rb->read_index = 0;
    rb->write_index = 0;
    int m = lcm(host_buffer_frames, user_buffer_frames);
    if (m == host_buffer_frames || m == user_buffer_frames) {
      m *= 2;
    }
    rb->buffer_frames = m;
    rb->v = calloc(rb->buffer_frames * channels, sizeof(float));
    if (!rb->v) {
      free(rb);
      rb = NULL;
    }
  }
  return rb;
}

static void release_buffer(bsa_ring_buffer *rb) {
  free(rb->v);
  free(rb);
}

buffer_size_adapter *bsa_create_adapter(
    int host_buffer_frames, int user_buffer_frames,
    int input_channels, int output_channels,
    audio_module_process_t user_process, void *user_context) {
  buffer_size_adapter *adapter = malloc(sizeof(buffer_size_adapter));
  if (adapter) {
    adapter->host_buffer_frames = host_buffer_frames;
    adapter->user_buffer_frames = user_buffer_frames;
    adapter->user_process = user_process;
    adapter->user_context = user_context;
    adapter->input_buffer =
      create_buffer(host_buffer_frames, user_buffer_frames, input_channels);
    adapter->output_buffer =
      create_buffer(host_buffer_frames, user_buffer_frames, output_channels);
    // TODO: Error checks.

    // Optimizing initial indices according to Stéphane Letz, "Callback
    // adaptation techniques"
    // (http://www.grame.fr/ressources/publications/CallbackAdaptation.pdf).
    int r, w = 0;
    int m = lcm(host_buffer_frames, user_buffer_frames);
    int dmax = 0;
    for (r = 0; r < m; r += host_buffer_frames) {
      for (; w < r; w += user_buffer_frames);
      int d = w - r;
      if (d > dmax) {
        dmax = d;
        adapter->output_buffer->read_index = r;
        adapter->output_buffer->write_index = w;
      }
    }
  }
  return adapter;
}

void bsa_release(buffer_size_adapter *adapter) {
  release_buffer(adapter->input_buffer);
  release_buffer(adapter->output_buffer);
  free(adapter);
}

void bsa_process(void *context, int sample_rate, int buffer_frames,
    int input_channels, const float *input_buffer,
    int output_channels, float *output_buffer) {
  buffer_size_adapter *adapter = (buffer_size_adapter *) context;

  // TODO: Write input samples to adapter->input_buffer.

  bsa_ring_buffer *ib = adapter->input_buffer;
  bsa_ring_buffer *ob = adapter->output_buffer;
  while (frames_available(adapter->input_buffer) >=
      adapter->user_buffer_frames) {
    adapter->user_process(adapter->user_context, sample_rate,
        adapter->user_buffer_frames,
        input_channels, ib->v + ib->read_index * input_channels,
        output_channels, ob->v + ob->write_index * output_channels);
    ib->read_index =
      (ib->read_index + adapter->user_buffer_frames) % ib->buffer_frames;
    ob->write_index =
      (ob->write_index + adapter->user_buffer_frames) % ob->buffer_frames;
  }

  // TODO: Write output samples to output_buffer.
}
