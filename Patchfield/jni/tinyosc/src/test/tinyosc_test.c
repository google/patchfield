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

#include "tinyosc.h"

#include "test_utils.h"

#define CAPACITY 256

static int buffers_match(const char *p, const char *q, int n) {
  int i;
  for (i = 0; i < n; ++i) {
    if (p[i] != q[i]) {
      int j;
      for (j = 0; j < n; ++j) {
        fprintf(stderr, "%x ", p[j]);
      }
      fprintf(stderr, "\n");
      for (j = 0; j < n; ++j) {
        fprintf(stderr, "%x ", q[j]);
      }
      fprintf(stderr, "\n");
      return 0;
    }
  }
  return 1;
}

static int test_pack_errors() {
  char data[CAPACITY];
  osc_packet packet;
  packet.data = data;

  EXPECT(osc_pack_message(&packet, CAPACITY, "", "") != 0);
  EXPECT(osc_pack_message(&packet, CAPACITY, "#", "") != 0);
  EXPECT(osc_pack_message(&packet, CAPACITY, "#foo", "") != 0);
  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo", "_") != 0);
  
  return 0;
}

static int test_pack_capacity() {
  char data[CAPACITY];
  osc_packet packet;
  packet.data = data;

  EXPECT(osc_pack_message(&packet, 0, "/ab", "") != 0);
  EXPECT(osc_pack_message(&packet, 7, "/ab", "") != 0);
  EXPECT(osc_pack_message(&packet, 8, "/ab", "") == 0);
  EXPECT(osc_pack_message(&packet, 11, "/ab", "") == 0);
  EXPECT(osc_pack_message(&packet, 7, "/foo", "") != 0);
  EXPECT(osc_pack_message(&packet, 8, "/foo", "") != 0);
  EXPECT(osc_pack_message(&packet, 12, "/foo", "") == 0);
  EXPECT(osc_pack_message(&packet, 15, "/foo", "") == 0);
  EXPECT(osc_pack_message(&packet, 15, "/foo", "i", 0) != 0);
  EXPECT(osc_pack_message(&packet, 16, "/foo", "i", 0) == 0);
  EXPECT(osc_pack_message(&packet, 15, "/foo", "f", 1.5) != 0);
  EXPECT(osc_pack_message(&packet, 16, "/foo", "f", 1.5) == 0);
  EXPECT(osc_pack_message(&packet, 15, "/foo", "s", "abc") != 0);
  EXPECT(osc_pack_message(&packet, 16, "/foo", "s", "abc") == 0);
  EXPECT(osc_pack_message(&packet, 19, "/foo", "s", "abcd") != 0);
  EXPECT(osc_pack_message(&packet, 20, "/foo", "s", "abcd") == 0);
  EXPECT(osc_pack_message(&packet, 23, "/foo", "b", 5, "abcde") != 0);
  EXPECT(osc_pack_message(&packet, 24, "/foo", "b", 5, "abcde") == 0);
  EXPECT(osc_pack_message(&packet, 19, "/foo", "ii", 0, 0) != 0);
  EXPECT(osc_pack_message(&packet, 20, "/foo", "ii", 0, 0) == 0);
  EXPECT(osc_pack_message(&packet, 19, "/foo", "fi", 1.5, 0) != 0);
  EXPECT(osc_pack_message(&packet, 20, "/foo", "fi", 1.5, 0) == 0);
  EXPECT(osc_pack_message(&packet, 19, "/foo", "si", "abc", 0) != 0);
  EXPECT(osc_pack_message(&packet, 20, "/foo", "si", "abc", 0) == 0);
  EXPECT(osc_pack_message(&packet, 27, "/foo", "bi", 5, "abcde", 0) != 0);
  EXPECT(osc_pack_message(&packet, 28, "/foo", "bi", 5, "abcde", 0) == 0);

  return 0;
}

static int test_pack_no_args() {
  char data[CAPACITY];
  osc_packet packet;
  packet.data = data;

  EXPECT(osc_pack_message(&packet, CAPACITY, "/", "") == 0);

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "") == 0);
  EXPECT(packet.size == 8);
  char ref0[] = { '/', 'a', 'b', 0, ',', 0, 0, 0 };
  EXPECT(buffers_match(ref0, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo/bar", "") == 0);
  EXPECT(packet.size == 16);
  char ref1[] = {
    '/', 'f', 'o', 'o', '/', 'b', 'a', 'r', 0, 0, 0, 0,
    ',', 0, 0, 0
  };
  EXPECT(buffers_match(ref1, packet.data, packet.size));

  return 0;
}

static int test_pack_one_arg() {
  char data[CAPACITY];
  osc_packet packet;
  packet.data = data;

  EXPECT(osc_pack_message(&packet, 37, "/ab", "i", 0x12345678) == 0);
  EXPECT(packet.size == 12);
  char ref0[] = {
    '/', 'a', 'b', 0, ',', 'i', 0, 0,
    0x12, 0x34, 0x56, 0x78
  };
  EXPECT(buffers_match(ref0, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, 43, "/abc", "m", 0x01020304) == 0);
  EXPECT(packet.size == 16);
  char ref1[] = {
    '/', 'a', 'b', 'c', 0, 0, 0, 0, ',', 'm', 0, 0,
    0x01, 0x02, 0x03, 0x04
  };
  EXPECT(buffers_match(ref1, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "f", 1.234) == 0);
  EXPECT(packet.size == 12);
  char ref2[] = {
    '/', 'a', 'b', 0, ',', 'f', 0, 0,
    0x3f, 0x9d, 0xf3, 0xb6
  };
  EXPECT(buffers_match(ref2, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "s", "") == 0);
  EXPECT(packet.size == 12);
  char ref3[] = {
    '/', 'a', 'b', 0, ',', 's', 0, 0,
    0, 0, 0, 0
  };
  EXPECT(buffers_match(ref3, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "s", "xyz") == 0);
  EXPECT(packet.size == 12);
  char ref4[] = {
    '/', 'a', 'b', 0, ',', 's', 0, 0,
    'x', 'y', 'z', 0
  };
  EXPECT(buffers_match(ref4, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "s", "abcdefg") == 0);
  EXPECT(packet.size == 16);
  char ref5[] = {
    '/', 'a', 'b', 0, ',', 's', 0, 0,
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 0
  };
  EXPECT(buffers_match(ref5, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "b", 0, NULL) == 0);
  EXPECT(packet.size == 12);
  char ref6[] = {
    '/', 'a', 'b', 0, ',', 'b', 0, 0,
    0, 0, 0, 0
  };
  EXPECT(buffers_match(ref6, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "b", 2, "abcdef") == 0);
  EXPECT(packet.size == 16);
  char ref7[] = {
    '/', 'a', 'b', 0, ',', 'b', 0, 0,
    0, 0, 0, 2, 'a', 'b', 0, 0
  };
  EXPECT(buffers_match(ref7, packet.data, packet.size));

  return 0;
}

static int test_pack_two_args() {
  char data[CAPACITY];
  osc_packet packet;
  packet.data = data;

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "ii", 0x01020304, -2) == 0);
  EXPECT(packet.size == 16);
  char ref0[] = {
    '/', 'a', 'b', 0, ',', 'i', 'i', 0,
    0x01, 0x02, 0x03, 0x04,
    0xff, 0xff, 0xff, 0xfe
  };
  EXPECT(buffers_match(ref0, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "fi", 440.0, -2) == 0);
  EXPECT(packet.size == 16);
  char ref1[] = {
    '/', 'a', 'b', 0, ',', 'f', 'i', 0,
    0x43, 0xdc, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xfe
  };
  EXPECT(buffers_match(ref1, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "si", "x", -2) == 0);
  EXPECT(packet.size == 16);
  char ref2[] = {
    '/', 'a', 'b', 0, ',', 's', 'i', 0,
    'x', 0, 0, 0,
    0xff, 0xff, 0xff, 0xfe
  };
  EXPECT(buffers_match(ref2, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "bi", 2, "abc", -2) == 0);
  EXPECT(packet.size == 20);
  char ref3[] = {
    '/', 'a', 'b', 0, ',', 'b', 'i', 0,
    0x00, 0x00, 0x00, 0x02,
    'a', 'b', 0, 0,
    0xff, 0xff, 0xff, 0xfe
  };
  EXPECT(buffers_match(ref3, packet.data, packet.size));

  return 0;
}

static int test_unpack_match() {
  osc_packet packet;
  packet.size = 4;
  packet.data = "/xy";

  EXPECT(osc_unpack_message(&packet, "/xyz", "") != 0);
  EXPECT(osc_unpack_message(&packet, "/xy", "i", NULL) != 0);
  EXPECT(osc_unpack_message(&packet, "/xy", "") == 0);

  char data[CAPACITY];
  packet.data = data;

  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo/*/bar", "") == 0);

  // Non-matching addresses.
  EXPECT(osc_unpack_message(&packet, "/foo", "") != 0);
  EXPECT(osc_unpack_message(&packet, "/foo/bar", "") != 0);
  EXPECT(osc_unpack_message(&packet, "/fooo/x/bar", "") != 0);
  EXPECT(osc_unpack_message(&packet, "/foo/x/baz", "") != 0);

  // Matching addresses.
  EXPECT(osc_unpack_message(&packet, "/foo//bar", "") == 0);
  EXPECT(osc_unpack_message(&packet, "/foo/x/bar", "") == 0);

  return 0;
}

static int test_unpack_one_arg() {
  char data[CAPACITY];
  osc_packet packet;
  packet.data = data;

  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo/*", "i", 42) == 0);
  int i;
  EXPECT(osc_unpack_message(&packet, "/foo/bar", "f", NULL) != 0);
  EXPECT(osc_unpack_message(&packet, "/foo/bar", "i", &i) == 0);
  EXPECT(i == 42);

  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo/*", "f", -0.5) == 0);
  float f;
  EXPECT(osc_unpack_message(&packet, "/foo/bar", "f", &f) == 0);
  EXPECT(f == -0.5);

  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo/*", "s", "") == 0);
  char s[16];
  EXPECT(osc_unpack_message(&packet, "/foo/bar", "s", s) == 0);
  EXPECT(!strcmp("", s));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo/*", "s", "bla") == 0);
  EXPECT(osc_unpack_message(&packet, "/foo/bar", "s", s) == 0);
  EXPECT(!strcmp("bla", s));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo/*", "b", 0, NULL) == 0);
  int n;
  EXPECT(osc_unpack_message(&packet, "/foo/bar", "b", &n, s) == 0);
  EXPECT(n == 0);

  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo/*", "b", 2, "xy") == 0);
  EXPECT(osc_unpack_message(&packet, "/foo/bar", "b", &n, s) == 0);
  EXPECT(n == 2);
  EXPECT(buffers_match("xy", s, n));

  return 0;
}

static int test_unpack_two_args() {
  char data[CAPACITY];
  osc_packet packet;
  packet.data = data;

  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo/*", "ii", 42, -3) == 0);
  int i, j;
  EXPECT(osc_unpack_message(&packet, "/foo/bar", "i", NULL) != 0);
  EXPECT(osc_unpack_message(&packet, "/foo/bar", "if", NULL, NULL) != 0);
  EXPECT(osc_unpack_message(&packet, "/foo/bar", "ii", &i, &j) == 0);
  EXPECT(i == 42);
  EXPECT(j == -3);

  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo/*", "if", 7, 1.234) == 0);
  float f;
  EXPECT(osc_unpack_message(&packet, "/foo/bar", "if", &i, &f) == 0);
  EXPECT(i == 7);
  EXPECT(f == 1.234f);

  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo/*", "si", "text", 1) == 0);
  char s[16];
  EXPECT(osc_unpack_message(&packet, "/foo/bar", "si", &s, &i) == 0);
  EXPECT(i == 1);
  EXPECT(!strcmp("text", s));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo/*",
        "mb", 0x00804860, 3, "text") == 0);
  EXPECT(osc_unpack_message(&packet, "/foo/bar", "mb", &i, &j, s) == 0);
  EXPECT(i == 0x00804860);
  EXPECT(j == 3);
  EXPECT(!strncmp("text", s, j));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/xy", "si", "cat", -7) == 0);
  EXPECT(osc_unpack_message(&packet, "/xy", "si", s, &i) == 0);
  EXPECT(!strcmp("cat", s));
  EXPECT(i == -7);

  EXPECT(osc_pack_message(&packet, CAPACITY, "/xy", "si", "cats", -7) == 0);
  EXPECT(osc_unpack_message(&packet, "/xy", "si", s, &i) == 0);
  EXPECT(!strcmp("cats", s));
  EXPECT(i == -7);
  return 0;
}

static int test_bundle_basics() {
  char data[CAPACITY];
  osc_packet packet;
  packet.data = data;

  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo", "") == 0);
  EXPECT(!osc_is_bundle(&packet));
  EXPECT(osc_time_from_bundle(&packet, NULL) != 0);
  EXPECT(osc_make_bundle(&packet, 4, 1) != 0);
  EXPECT(osc_make_bundle(&packet, 16, 0x0102030405060708) == 0);
  EXPECT(osc_is_bundle(&packet));
  EXPECT(packet.size == 16);
  char ref[] = {
    '#', 'b', 'u', 'n', 'd', 'l', 'e', 0,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
  };
  EXPECT(buffers_match(ref, packet.data, packet.size));
  int64_t time;
  EXPECT(osc_time_from_bundle(&packet, &time) == 0);
  EXPECT(time == 0x0102030405060708);

  return 0;
}

static int test_bundle_add_and_get() {
  char data0[CAPACITY];
  osc_packet bundle;
  bundle.data = data0;
  char data1[CAPACITY];
  osc_packet packet;
  packet.data = data1;
  osc_packet extracted = { 0, NULL };

  EXPECT(osc_make_bundle(&bundle, CAPACITY, 0x0102030405060708) == 0);
  EXPECT(osc_next_packet_from_bundle(&bundle, NULL) != 0);
  EXPECT(extracted.data == NULL);

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "i", 0x12345678) == 0);
  EXPECT(osc_add_packet_to_bundle(&bundle, CAPACITY, &packet) == 0);
  EXPECT(bundle.size == 32);
  char ref0[] = {
    '#', 'b', 'u', 'n', 'd', 'l', 'e', 0,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x00, 0x00, 0x00, 0x0c,
    '/', 'a', 'b', 0, ',', 'i', 0, 0, 0x12, 0x34, 0x56, 0x78
  };
  EXPECT(buffers_match(ref0, bundle.data, bundle.size));

  extracted.size = 0;
  extracted.data = NULL;
  EXPECT(osc_next_packet_from_bundle(&bundle, &extracted) == 0);
  EXPECT(extracted.size == 12);
  char ref1[] = {
    '/', 'a', 'b', 0, ',', 'i', 0, 0, 0x12, 0x34, 0x56, 0x78
  };
  EXPECT(buffers_match(ref1, extracted.data, extracted.size));
  EXPECT(osc_next_packet_from_bundle(&bundle, &extracted) != 0);

  EXPECT(osc_pack_message(&packet, CAPACITY, "/abc", "f", 1.234) == 0);
  EXPECT(osc_add_packet_to_bundle(&bundle, CAPACITY, &packet) == 0);
  EXPECT(bundle.size == 52);
  char ref2[] = {
    '#', 'b', 'u', 'n', 'd', 'l', 'e', 0,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x00, 0x00, 0x00, 0x0c,
    '/', 'a', 'b', 0, ',', 'i', 0, 0, 0x12, 0x34, 0x56, 0x78,
    0x00, 0x00, 0x00, 0x10,
    '/', 'a', 'b', 'c', 0, 0, 0, 0, ',', 'f', 0, 0, 0x3f, 0x9d, 0xf3, 0xb6
  };
  EXPECT(buffers_match(ref2, bundle.data, bundle.size));

  extracted.size = 0;
  extracted.data = NULL;
  EXPECT(osc_next_packet_from_bundle(&bundle, &extracted) == 0);
  EXPECT(extracted.size == 12);
  EXPECT(buffers_match(ref1, extracted.data, extracted.size));

  EXPECT(osc_next_packet_from_bundle(&bundle, &extracted) == 0);
  EXPECT(packet.size == 16);
  char ref3[] = {
    '/', 'a', 'b', 'c', 0, 0, 0, 0, ',', 'f', 0, 0, 0x3f, 0x9d, 0xf3, 0xb6
  };
  EXPECT(buffers_match(ref3, extracted.data, extracted.size));

  EXPECT(osc_next_packet_from_bundle(&bundle, &extracted) != 0);

  return 0;
}

static int test_message_to_string_no_args() {
  char s[CAPACITY];
  char data[CAPACITY];
  osc_packet packet;

  packet.size = 8;
  packet.data = "/xyz";
  EXPECT(osc_message_to_string(s, CAPACITY, &packet) == 4);
  EXPECT(!strcmp(s, "/xyz"));
  EXPECT(osc_message_to_string(s, 5, &packet) == 4);
  EXPECT(!strcmp(s, "/xyz"));
  EXPECT(osc_message_to_string(s, 4, &packet) == 4);
  EXPECT(!strcmp(s, "/xy"));

  packet.data = data;
  EXPECT(osc_pack_message(&packet, CAPACITY, "/abcd", "") == 0);
  EXPECT(osc_message_to_string(s, CAPACITY, &packet) == 5);
  EXPECT(!strcmp(s, "/abcd"));
  EXPECT(osc_message_to_string(s, 6, &packet) == 5);
  EXPECT(!strcmp(s, "/abcd"));
  EXPECT(osc_message_to_string(s, 5, &packet) == 5);
  EXPECT(!strcmp(s, "/abc"));

  return 0;
}

static int test_message_to_string_one_arg() {
  char s[CAPACITY];
  char data[CAPACITY];
  osc_packet packet;
  packet.data = data;

  EXPECT(osc_pack_message(&packet, CAPACITY, "/xy", "i", -7) == 0);
  EXPECT(osc_message_to_string(s, CAPACITY, &packet) == 8);
  EXPECT(!strcmp(s, "/xy i:-7"));
  EXPECT(osc_message_to_string(s, 9, &packet) == 8);
  EXPECT(!strcmp(s, "/xy i:-7"));
  EXPECT(osc_message_to_string(s, 8, &packet) == 8);
  EXPECT(!strcmp(s, "/xy i:-"));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/xy", "f", 3.5) == 0);
  EXPECT(osc_message_to_string(s, CAPACITY, &packet) == 14);
  EXPECT(!strcmp(s, "/xy f:3.500000"));
  EXPECT(osc_message_to_string(s, 15, &packet) == 14);
  EXPECT(!strcmp(s, "/xy f:3.500000"));
  EXPECT(osc_message_to_string(s, 14, &packet) == 14);
  EXPECT(!strcmp(s, "/xy f:3.50000"));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/xy", "s", "mouse") == 0);
  EXPECT(osc_message_to_string(s, CAPACITY, &packet) == 11);
  EXPECT(!strcmp(s, "/xy s:mouse"));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/xy", "b", 3, "zzz") == 0);
  EXPECT(osc_message_to_string(s, CAPACITY, &packet) == 7);
  EXPECT(!strcmp(s, "/xy b:3"));

  return 0;
}

static int test_message_to_string_two_args() {
  char s[CAPACITY];
  char data[CAPACITY];
  osc_packet packet;
  packet.data = data;

  EXPECT(osc_pack_message(&packet, CAPACITY, "/xy", "ii", 5, -7) == 0);
  EXPECT(osc_message_to_string(s, CAPACITY, &packet) == 12);
  EXPECT(!strcmp(s, "/xy i:5 i:-7"));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/xy", "fi", 2.5, -7) == 0);
  EXPECT(osc_message_to_string(s, CAPACITY, &packet) == 19);
  EXPECT(!strcmp(s, "/xy f:2.500000 i:-7"));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/xy", "mi", 0x11223344, -7) == 0);
  EXPECT(osc_message_to_string(s, CAPACITY, &packet) == 19);
  EXPECT(!strcmp(s, "/xy m:11223344 i:-7"));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/xy", "si", "cat", -7) == 0);
  EXPECT(osc_message_to_string(s, CAPACITY, &packet) == 14);
  EXPECT(!strcmp(s, "/xy s:cat i:-7"));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/xy", "si", "cats", -7) == 0);
  EXPECT(osc_message_to_string(s, CAPACITY, &packet) == 15);
  EXPECT(!strcmp(s, "/xy s:cats i:-7"));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/xy",
        "bi", 8, "01234567", -7) == 0);
  EXPECT(osc_message_to_string(s, CAPACITY, &packet) == 12);
  EXPECT(!strcmp(s, "/xy b:8 i:-7"));

  return 0;
}

int main(int argc, char **argv) {
  TEST(test_pack_errors);
  TEST(test_pack_capacity);
  TEST(test_pack_no_args);
  TEST(test_pack_one_arg);
  TEST(test_pack_two_args);
  TEST(test_unpack_match);
  TEST(test_unpack_one_arg);
  TEST(test_unpack_two_args);
  TEST(test_bundle_basics);
  TEST(test_bundle_add_and_get);
  TEST(test_message_to_string_no_args);
  TEST(test_message_to_string_one_arg);
  TEST(test_message_to_string_two_args);
}
