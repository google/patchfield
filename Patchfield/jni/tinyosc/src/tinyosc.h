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

// A minimal but complete implementation of Open Sound Control 1.0
// (http://opensoundcontrol.org/spec-1_0), providing serialization and
// deserialization of OSC messages but no networking support.  This
// implementation is real-time safe; in particular, it does not perform any
// memory allocation. The functions for packing and unpacking OSC packets are
// supposed to be reminiscent of the venerable printf/scanf functions in plain
// C.

#ifndef __TINYOSC_H__
#define __TINYOSC_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

// Data structure representing OSC packets. The data field points to a buffer
// that holds a serialized OSC packet. The size field indicates the size of the
// serialized packet, _not_ the capacity of the buffer. API functions that need
// to know the capacity of the buffer take an additional capacity parameter.
//
// A note on byte order: The size field is in the host byte order, i.e., client
// code can use it directly without having to convert it. Per OSC
// specification, all numbers in the data buffer are in the network byte order,
// but client code won't access them directly. API functions take care of the
// byte order under the hood, and so client code will not need to deal with
// byte order issues.
typedef struct {
  int32_t size;  // Must be a multiple of 4.
  char *data;
} osc_packet;

// Packs the given parameters into an OSC message; returns 0 on success. The
// address string is an OSC address pattern, i.e., it can contain wild cards.
// packet->data must point to a buffer whose capacity is at least the value of
// the capacity parameter.
//
// The types string is an OSC type tag string _without the leading comma_.  For
// example, if you want to pack a float and an integer, then types will be just
// "fi".
//
// OSC type     Tag    C type
//   Integer    'i'    int32_t
//   Float      'f'    float
//   String     's'    const char*
//   Blob       'b'    int32_t (size of blob), const char*
//   MIDI       'm'    int32_t
int osc_pack_message(osc_packet *packet, int capacity,
    const char *address, const char *types, ...);

// Unpacks the given OSC message if its address pattern matches the given
// address and its types string matches the given types string; returns 0 on
// success.
//
// The types string is an OSC type tag string _without the leading comma_. For
// example, if you want to unpack a float and an integer, then types will be
// just "fi".
//
// OSC type     Tag    C type
//   Integer    'i'    int32_t*
//   Float      'f'    float*
//   String     's'    char*
//   Blob       'b'    int32_t* (size of blob), char*
//   MIDI       'm'    int32_t*
//
// When extracting strings or blobs from an OSC message, make sure to pass in
// a pointer to a sufficiently large char buffer. The size of the message
// provides a quick a-priori upper bound on the required size of buffers.
//
// Typical call pattern:
//   float x, y;
//   if (!osc_unpack_message(&packet, "/xy", "ff", &x, &y)) {
//     // Handle coordinates from xy-pad.
//   }
int osc_unpack_message(const osc_packet *packet,
    const char *address, const char *types, ...);

// Returns true if the given packet is an OSC bundle.
int osc_is_bundle(const osc_packet *packet);

// Creates an empty bundle with the given OSC time tag; returns 0 on success.
// bundle->data must point to a buffer whose capacity is at least the value of
// the capacity parameter. The time parameter must be in host byte order;
// conversion to network byte order happens under the hood.
int osc_make_bundle(osc_packet *bundle, int capacity, uint64_t time);

// Adds the given OSC packet to the given bundle; returns 0 on success.
int osc_add_packet_to_bundle(
    osc_packet *bundle, int capacity, const osc_packet *packet);

// Extracts the OSC time tag from the given bundle; returns 0 on success. The
// value written to *time will be in host byte order; conversion from network
// byte order happens under the hood.
int osc_time_from_bundle(const osc_packet *bundle, uint64_t *time);

// Iterator for stepping through the contents of an OSC bundle. When
// current->data is NULL, it writes the first packet in the bundle to current.
// On the next invocation, it writes the second packet to current, and so on.
// Returns 0 on success. Keep in mind that OSC bundles may contain bundles.
//
// Typical call pattern:
//   osc_packet current = { 0, NULL };
//   while (!osc_next_packet_from_bundle(&bundle, &current)) {
//     if (osc_is_bundle(&current)) {
//       // Recursively unpack sub-bundle.
//     } else {
//       // Handle message.
//     }
//   }
int osc_next_packet_from_bundle(const osc_packet *bundle, osc_packet *current);

// Computes a string representation of the given OSC message and writes the
// result to the string s; mostly for testing and debugging. The capacity is
// the maximum number of bytes in s to be used, including the terminating null
// character.
//
// This function mimics the behavior of snprintf, i.e., the return value is the
// number of characters that would have been written if the capacity had been
// sufficiently large, or a negative error code if the OSC message is
// malformed.
//
// Note that this function only accepts OSC _messages_, not bundles.
int osc_message_to_string(char *s, int capacity, const osc_packet *message);

#endif
