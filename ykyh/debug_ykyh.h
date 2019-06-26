/*
 * Copyright 2015-2019 Yubico AB
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DEBUG_YKYH_H
#define DEBUG_YKYH_H

#include <stdio.h>
#include <stdlib.h>

#include "../common/debug.h"

// FIXME(adma): hardcoding state->{verbodity, debug_file} is not very nice ...

#define DBG_INFO(...)                                                          \
  do {                                                                         \
    DLN(state->verbosity &YKYH_VERB_INFO, state->debug_file, ANSI_BLUE,        \
        "YKYH", "INF", __VA_ARGS__);                                           \
  } while (0)

#define DBG_WARN(...)                                                          \
  do {                                                                         \
    DLN(state->verbosity &YKYH_VERB_WARN, state->debug_file, ANSI_YELLOW,      \
        "YKYH", "WRN", __VA_ARGS__);                                           \
  } while (0)

#define DBG_ERR(...)                                                           \
  do {                                                                         \
    DLN(state->verbosity &YKYH_VERB_ERR, state->debug_file, ANSI_RED, "YKYH",  \
        "ERR", __VA_ARGS__);                                                   \
  } while (0)
#endif
