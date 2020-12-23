/*
 * Copyright 2015-2018 Yubico AB
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

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <ykyh.h>

#include <openssl/evp.h>

#include "parsing.h"

#include "cmdline.h"

static bool parse_name(const char *prompt, char *name, char *parsed,
                       size_t *parsed_len) {
  if (strlen(name) > *parsed_len) {
    fprintf(stdout, "Unable to read name, buffer too small\n");
    return false;
  }

  if (strlen(name) == 0) {
    if (read_string(prompt, parsed, *parsed_len, false) == false) {
      return false;
    }
  } else {
    strncpy(parsed, name, strlen(name));
  }

  *parsed_len = strlen(name);

  return true;
}

static bool parse_pw(const char *prompt, char *pw, char *parsed,
                     size_t *parsed_len) {
  if (strlen(pw) > *parsed_len) {
    fprintf(stderr, "Unable to read password, buffer too small\n");
    return false;
  }

  if (strlen(pw) == 0) {
    if (read_string(prompt, parsed, *parsed_len, HIDDEN_CHECKED) == false) {
      return false;
    }
  } else {
    strncpy(parsed, pw, *parsed_len);
  }

  *parsed_len = strlen(parsed);

  return true;
}

static bool parse_key(const char *prompt, char *key, uint8_t *parsed,
                      size_t *parsed_len) {
  char buf[128];
  size_t buf_size = sizeof(buf);

  if (strlen(key) > buf_size) {
    fprintf(stdout, "Unable to read key, buffer too small\n");
    return false;
  }

  if (strlen(key) == 0) {
    if (read_string(prompt, buf, buf_size, true) == false) {
      return false;
    }
    buf_size = strlen(buf);
  } else {
    memcpy(buf, key, strlen(key));
    buf_size = strlen(key);
  }

  if (hex_decode(buf, parsed, parsed_len) == false) {
    return false;
  }

  if (*parsed_len != YKYH_KEY_LEN) {
    fprintf(stdout, "Unable to read key, wrong length (must be %d)\n",
            YKYH_KEY_LEN);
    return false;
  }

  return true;
}

static bool parse_context(const char *prompt, char *context, uint8_t *parsed,
                          size_t *parsed_len) {
  char buf[128];
  size_t buf_size = sizeof(buf);

  if (strlen(context) > buf_size) {
    fprintf(stdout, "Unable to read context, buffer too small\n");
    return false;
  }

  if (strlen(context) == 0) {
    if (read_string(prompt, buf, buf_size, false) == false) {
      return false;
    }
    buf_size = strlen(buf);
  } else {
    memcpy(buf, context, strlen(context));
    buf_size = strlen(context);
  }

  if (hex_decode(buf, parsed, parsed_len) == false) {
    return false;
  }

  if (*parsed_len != YKYH_CONTEXT_LEN) {
    fprintf(stdout, "Unable to read context, wrong length (must be %d)\n",
            YKYH_CONTEXT_LEN);
    return false;
  }

  return true;
}

static bool parse_touch_policy(enum enum_touch touch_policy,
                               uint8_t *touch_policy_parsed) {

  switch (touch_policy) {
    case touch__NULL:
    case touch_arg_off:
      *touch_policy_parsed = 0;
      break;

    case touch_arg_on:
      *touch_policy_parsed = 1;
      break;
  }

  return true;
}

static bool delete_credential(ykyh_state *state, char *authkey, char *name) {
  ykyh_rc ykyhrc;
  uint8_t authkey_parsed[YKYH_PW_LEN];
  size_t authkey_parsed_len = sizeof(authkey_parsed);
  char name_parsed[YKYH_MAX_NAME_LEN + 2] = {0};
  size_t name_parsed_len = sizeof(name_parsed);
  uint8_t retries;

  if (parse_key("Authentication key", authkey, authkey_parsed,
                &authkey_parsed_len) == false) {
    return false;
  }

  if (parse_name("Name", name, name_parsed, &name_parsed_len) == false) {
    return false;
  }

  ykyhrc = ykyh_delete(state, authkey_parsed, authkey_parsed_len, name_parsed,
                       &retries);
  if (ykyhrc != YKYHR_SUCCESS) {
    fprintf(stderr, "Unable to delete credential: %s, %d retries left\n",
            ykyh_strerror(ykyhrc), retries);
    return false;
  }

  fprintf(stdout, "Credential successfully deleted\n");

  return true;
}

static bool list_credentials(ykyh_state *state) {
  ykyh_rc ykyhrc;
  ykyh_list_entry list[32];
  size_t list_items = sizeof(list) / sizeof(list[0]);

  ykyhrc = ykyh_list_keys(state, list, &list_items);
  if (ykyhrc != YKYHR_SUCCESS) {
    fprintf(stderr, "Unable to list credentials: %s\n", ykyh_strerror(ykyhrc));
    return false;
  }

  if (list_items == 0) {
    fprintf(stdout, "No items found\n");
    return true;
  }

  fprintf(stdout, "Found %lu item(s)\n", list_items);
  fprintf(stdout, "Algo\tTouch\tCounter\tName\n");

  for (size_t i = 0; i < list_items; i++) {
    fprintf(stdout, "%d\t%d\t%d\t%s\n", list[i].algo, list[i].touch,
            list[i].ctr, list[i].name);
  }

  return true;
}

static bool put_credential(ykyh_state *state, char *authkey, char *name,
                           char *derivation_password, char *key_enc,
                           char *key_mac, char *password,
                           enum enum_touch touch_policy) {
  ykyh_rc ykyhrc;
  uint8_t authkey_parsed[YKYH_KEY_LEN];
  size_t authkey_parsed_len = sizeof(authkey_parsed);
  char name_parsed[YKYH_MAX_NAME_LEN + 2] = {0};
  size_t name_parsed_len = sizeof(name_parsed);
  char dpw_parsed[256] = {0};
  size_t dpw_parsed_len = sizeof(dpw_parsed);
  uint8_t key_enc_parsed[YKYH_KEY_LEN];
  size_t key_enc_parsed_len = sizeof(key_enc_parsed);
  uint8_t key_mac_parsed[YKYH_KEY_LEN];
  size_t key_mac_parsed_len = sizeof(key_mac_parsed);
  char pw_parsed[YKYH_PW_LEN + 2] = {0};
  size_t pw_parsed_len = sizeof(pw_parsed);
  uint8_t touch_policy_parsed = 0;
  uint8_t retries;

  if (parse_key("Authentication key", authkey, authkey_parsed,
                &authkey_parsed_len) == false) {
    return false;
  }

  if (parse_name("Name", name, name_parsed, &name_parsed_len) == false) {
    return false;
  }

  if (strlen(key_mac) == 0 && strlen(key_enc) == 0) {
    if (parse_pw("Derivation password", derivation_password, dpw_parsed,
                 &dpw_parsed_len) == false) {
      return false;
    }
  } else {
    dpw_parsed_len = 0;
  }

  if (dpw_parsed_len == 0) {
    if (parse_key("Encryption key", key_enc, key_enc_parsed,
                  &key_enc_parsed_len) == false) {
      return false;
    }

    if (parse_key("MAC key", key_mac, key_mac_parsed, &key_mac_parsed_len) ==
        false) {
      return false;
    }
  } else {
    uint8_t key[YKYH_KEY_LEN * 2];
    int ret = PKCS5_PBKDF2_HMAC((const char *) dpw_parsed, dpw_parsed_len,
                                (uint8_t *) YKYH_DEFAULT_SALT,
                                strlen(YKYH_DEFAULT_SALT), YKYH_DEFAULT_ITERS,
                                EVP_sha256(), sizeof(key), key);

    if (ret != 1) {
      return false;
    }

    memcpy(key_enc_parsed, key, YKYH_KEY_LEN);
    key_enc_parsed_len = YKYH_KEY_LEN;
    memcpy(key_mac_parsed, key + YKYH_KEY_LEN, YKYH_KEY_LEN);
    key_mac_parsed_len = YKYH_KEY_LEN;
  }

  if (parse_pw("Credential Password (max 16 characters)", password, pw_parsed,
               &pw_parsed_len) == false) {
    return false;
  }

  if (pw_parsed_len > YKYH_PW_LEN) {
    fprintf(stderr, "Credential password can not be more than %d characters.\n",
            YKYH_PW_LEN);
    return false;
  }

  if (parse_touch_policy(touch_policy, &touch_policy_parsed) == false) {
    return false;
  }

  ykyhrc =
    ykyh_put(state, authkey_parsed, authkey_parsed_len, name_parsed,
             key_enc_parsed, key_enc_parsed_len, key_mac_parsed,
             key_mac_parsed_len, pw_parsed, touch_policy_parsed, &retries);
  if (ykyhrc != YKYHR_SUCCESS) {
    fprintf(stderr, "Unable to store credential: %s, %d retries left\n",
            ykyh_strerror(ykyhrc), retries);
    return false;
  }

  fprintf(stdout, "Credential successfully stored\n");

  return true;
}

bool reset_device(ykyh_state *state) {
  ykyh_rc ykyhrc;

  ykyhrc = ykyh_reset(state);
  if (ykyhrc != YKYHR_SUCCESS) {
    fprintf(stderr, "Unable to reset device: %s\n", ykyh_strerror(ykyhrc));
    return false;
  }

  fprintf(stdout, "Device successuflly reset\n");

  return true;
}

bool get_authkey_retries(ykyh_state *state) {
  ykyh_rc ykyhrc;
  uint8_t retries;

  ykyhrc = ykyh_get_authkey_retries(state, &retries);
  if (ykyhrc != YKYHR_SUCCESS) {
    fprintf(stderr, "Unable to get authkey retries: %s\n",
            ykyh_strerror(ykyhrc));
    return false;
  }

  fprintf(stdout, "Retries left for Authentication Key: %d\n", retries);

  return true;
}

bool get_version(ykyh_state *state) {
  ykyh_rc ykyhrc;
  char version[64];
  size_t version_len = sizeof(version);

  ykyhrc = ykyh_get_version(state, version, version_len);
  if (ykyhrc != YKYHR_SUCCESS) {
    fprintf(stderr, "Unable to get version: %s\n", ykyh_strerror(ykyhrc));
    return false;
  }

  fprintf(stdout, "Version %s\n", version);

  return true;
}

void print_key(char *prompt, uint8_t *key, size_t len) {
  fprintf(stdout, "%s: ", prompt);
  for (size_t i = 0; i < len; i++) {
    fprintf(stdout, "%02x", key[i]);
  }
  fprintf(stdout, "\n");
}

static bool calculate_session_keys(ykyh_state *state, char *name,
                                   char *password, char *context) {
  ykyh_rc ykyhrc;
  char name_parsed[YKYH_MAX_NAME_LEN + 2] = {0};
  size_t name_parsed_len = sizeof(name_parsed);
  uint8_t context_parsed[YKYH_CONTEXT_LEN];
  size_t context_parsed_len = sizeof(context_parsed);
  char pw_parsed[YKYH_PW_LEN + 2] = {0};
  size_t pw_parsed_len = sizeof(pw_parsed);
  uint8_t key_s_enc[YKYH_KEY_LEN];
  uint8_t key_s_mac[YKYH_KEY_LEN];
  uint8_t key_s_rmac[YKYH_KEY_LEN];
  size_t key_s_enc_len = sizeof(key_s_enc);
  size_t key_s_mac_len = sizeof(key_s_mac);
  size_t key_s_rmac_len = sizeof(key_s_rmac);
  uint8_t retries;

  if (parse_name("Name", name, name_parsed, &name_parsed_len) == false) {
    return false;
  }

  if (parse_context("Context", context, context_parsed, &context_parsed_len) ==
      false) {
    return false;
  }

  if (parse_pw("Password", password, pw_parsed, &pw_parsed_len) == false) {
    return false;
  }

  ykyhrc =
    ykyh_calculate(state, name_parsed, context_parsed, context_parsed_len,
                   pw_parsed, key_s_enc, key_s_enc_len, key_s_mac,
                   key_s_mac_len, key_s_rmac, key_s_rmac_len, &retries);
  if (ykyhrc != YKYHR_SUCCESS) {
    fprintf(stderr, "Unable to calculate session keys: %s\n",
            ykyh_strerror(ykyhrc));
    if (ykyhrc == YKYHR_WRONG_PW) {
      fprintf(stderr, "%d attempts left\n", retries);
    }
    return false;
  }

  print_key("Session encryption key", key_s_enc, key_s_enc_len);
  print_key("Session MAC key", key_s_mac, key_s_mac_len);
  print_key("Session R-MAC key", key_s_rmac, key_s_rmac_len);

  return true;
}

static bool put_authkey(ykyh_state *state, char *authkey, char *new_authkey) {
  ykyh_rc ykyhrc;
  uint8_t authkey_parsed[YKYH_KEY_LEN];
  size_t authkey_parsed_len = sizeof(authkey_parsed);
  uint8_t new_authkey_parsed[YKYH_KEY_LEN];
  size_t new_authkey_parsed_len = sizeof(authkey_parsed);
  uint8_t retries;

  if (parse_key("Authentication key", authkey, authkey_parsed,
                &authkey_parsed_len) == false) {
    return false;
  }

  if (parse_key("New Authentication key", new_authkey, new_authkey_parsed,
                &new_authkey_parsed_len) == false) {
    return false;
  }

  ykyhrc =
    ykyh_put_authkey(state, authkey_parsed, authkey_parsed_len,
                     new_authkey_parsed, new_authkey_parsed_len, &retries);
  if (ykyhrc != YKYHR_SUCCESS) {
    fprintf(stderr,
            "Unable to change Authentication key: %s, %d retries left\n",
            ykyh_strerror(ykyhrc), retries);
    return false;
  }

  fprintf(stdout, "Authentication key successfully changed\n");

  return true;
}

int main(int argc, char *argv[]) {
  struct gengetopt_args_info args_info;
  ykyh_state *state = NULL;
  ykyh_rc ykyhrc;

  int rc = EXIT_FAILURE;

  if (cmdline_parser(argc, argv, &args_info) != 0) {
    goto main_exit;
  }

  ykyhrc = ykyh_init(&state, args_info.verbose_arg);
  if (ykyhrc != YKYHR_SUCCESS) {
    fprintf(stderr, "Failed to initialize libykyh\n");
    goto main_exit;
  }

  ykyhrc = ykyh_connect(state, args_info.reader_arg);
  if (ykyhrc != YKYHR_SUCCESS) {
    fprintf(stderr, "Unable to connect: %s\n", ykyh_strerror(ykyhrc));
    goto main_exit;
  }

  bool result = false;
  switch (args_info.action_arg) {
    case action_arg_calculate:
      result =
        calculate_session_keys(state, args_info.name_arg,
                               args_info.password_arg, args_info.context_arg);
      break;

    case action_arg_change:
      result = put_authkey(state, args_info.authkey_arg, args_info.newkey_arg);
      break;

    case action_arg_delete:
      result =
        delete_credential(state, args_info.authkey_arg, args_info.name_arg);
      break;

    case action_arg_list:
      result = list_credentials(state);
      break;

    case action_arg_put:
      result = put_credential(state, args_info.authkey_arg, args_info.name_arg,
                              args_info.derivation_password_arg,
                              args_info.enckey_arg, args_info.mackey_arg,
                              args_info.password_arg, args_info.touch_arg);
      break;

    case action_arg_reset:
      result = reset_device(state);
      break;

    case action_arg_retries:
      result = get_authkey_retries(state);
      break;

    case action_arg_version:
      result = get_version(state);
      break;

    case action__NULL:
      fprintf(stderr, "No action given, nothing to do\n");
      break;
  }

  if (result == true) {
    rc = EXIT_SUCCESS;
  }

main_exit:

  ykyh_done(state);

  return rc;
}
