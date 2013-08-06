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

#include "buffer_size_adapter.h"

#include <stdlib.h>
#include <string.h>

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

static void transfer_buffers(int nframes, int nchannels,
    const float *source, int source_index, int source_frames,
    float *sink, int sink_index, int sink_frames) {
  int c, n, b;
  while (nframes > 0) {
    n = nframes;
    b = source_frames - source_index % source_frames;
    if (b < n) {
      n = b;
    }
    b = sink_frames - sink_index % sink_frames;
    if (b < n) {
      n = b;
    }
    for (c = 0; c < nchannels; ++c) {
      memcpy(
          sink + (sink_index / sink_frames) * sink_frames * nchannels +
          c * sink_frames + sink_index % sink_frames,
          source + (source_index / source_frames) * source_frames * nchannels +
          c * source_frames + source_index % source_frames,
          n * sizeof(float));
    }
    nframes -= n;
    source_index += n;
    sink_index += n;
  }
}

static void bsa_process(void *context, int sample_rate, int buffer_frames,
    int input_channels, const float *input_buffer,
    int output_channels, float *output_buffer) {
  buffer_size_adapter *adapter = (buffer_size_adapter *) context;
  if (adapter->host_buffer_frames != adapter->user_buffer_frames) {
    bsa_ring_buffer *ib = adapter->input_buffer;
    bsa_ring_buffer *ob = adapter->output_buffer;
    transfer_buffers(buffer_frames, input_channels,
        input_buffer, 0, buffer_frames,
        ib->v, ib->write_index, adapter->user_buffer_frames);
    ib->write_index = (ib->write_index + buffer_frames) % ib->buffer_frames;
    while (frames_available(ib) >= adapter->user_buffer_frames) {
      adapter->user_process(adapter->user_context, sample_rate,
          adapter->user_buffer_frames,
          input_channels, ib->v + ib->read_index * input_channels,
          output_channels, ob->v + ob->write_index * output_channels);
      ib->read_index =
        (ib->read_index + adapter->user_buffer_frames) % ib->buffer_frames;
      ob->write_index =
        (ob->write_index + adapter->user_buffer_frames) % ob->buffer_frames;
    }
    if (frames_available(ob) >= buffer_frames) {
      transfer_buffers(buffer_frames, output_channels,
          ob->v, ob->read_index, adapter->user_buffer_frames,
          output_buffer, 0, buffer_frames);
      ob->read_index = (ob->read_index + buffer_frames) % ob->buffer_frames;
    } else {
      memset(output_buffer, 0, buffer_frames * output_channels * sizeof(float));
    }
  } else {
    adapter->user_process(adapter->user_context, sample_rate, buffer_frames,
        input_channels, input_buffer, output_channels, output_buffer);
  }
}

static bsa_ring_buffer *create_buffer(
    int host_buffer_frames, int user_buffer_frames, int nchannels) {
  bsa_ring_buffer *rb = malloc(sizeof(bsa_ring_buffer));
  if (rb) {
    rb->read_index = 0;
    rb->write_index = 0;
    int m = lcm(host_buffer_frames, user_buffer_frames);
    if (m == host_buffer_frames || m == user_buffer_frames) {
      m *= 2;
    }
    rb->buffer_frames = m;
    rb->v = calloc(rb->buffer_frames * nchannels, sizeof(float));
    if (!rb->v) {
      free(rb);
      rb = NULL;
    }
  }
  return rb;
}

static void release_buffer(bsa_ring_buffer *rb) {
  if (rb) {
    free(rb->v);
  }
  free(rb);
}

buffer_size_adapter *bsa_create(
    void *handle, int host_buffer_frames, int user_buffer_frames,
    int input_channels, int output_channels,
    audio_module_process_t user_process, void *user_context) {
  buffer_size_adapter *adapter = malloc(sizeof(buffer_size_adapter));
  if (adapter) {
    adapter->host_buffer_frames = host_buffer_frames;
    adapter->user_buffer_frames = user_buffer_frames;
    adapter->user_process = user_process;
    adapter->user_context = user_context;
    if (host_buffer_frames != user_buffer_frames) {
      adapter->input_buffer =
        create_buffer(host_buffer_frames, user_buffer_frames, input_channels);
      if (adapter->input_buffer) {
        adapter->output_buffer = create_buffer(host_buffer_frames,
            user_buffer_frames, output_channels);
        if (adapter->output_buffer) {
          // Optimizing initial indices according to St\'ephane Letz, "Callback
          // adaptation techniques"
          // (see www.grame.fr/ressources/publications/CallbackAdaptation.pdf).
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
        } else {
          release_buffer(adapter->input_buffer);
          free(adapter);
          adapter = NULL;
        }
      } else {
        free(adapter);
        adapter = NULL;
      }
    } else {
      adapter->input_buffer = NULL;
      adapter->output_buffer = NULL;
    }
  }
  am_configure(handle, bsa_process, adapter);
  return adapter;
}

void bsa_release(buffer_size_adapter *adapter) {
  release_buffer(adapter->input_buffer);
  release_buffer(adapter->output_buffer);
  free(adapter);
}
