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

#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <stdio.h>

#define EXPECT(x) \
  do { \
    if (!(x)) { \
      fprintf(stderr, "\033[31;1m%s:%i failed.\033[0m %s\n", \
          __FUNCTION__, __LINE__, #x); \
      return -1; \
    } \
  } while (0)

#define TEST(x) \
  do { \
    if (!x()) fprintf(stderr, "\033[32;1m%s passed.\033[0m\n", #x); \
  } while (0)

#endif
