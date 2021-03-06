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

#ifndef YKHSMAUTH_H
#define YKHSMAUTH_H

#include <stdint.h>
#include <stddef.h>

//#include <ykhsmauth-version.h>

#ifdef __cplusplus
extern "C" {
#endif

// INS codes
#define YKHSMAUTH_INS_PUT 0x01
#define YKHSMAUTH_INS_DELETE 0x02
#define YKHSMAUTH_INS_CALCULATE 0x03
#define YKHSMAUTH_INS_GET_CHALLENGE 0x04
#define YKHSMAUTH_INS_LIST 0x05
#define YKHSMAUTH_INS_RESET 0x06
#define YKHSMAUTH_INS_GET_VERSION 0x07
#define YKHSMAUTH_INS_PUT_MGMKEY 0x08
#define YKHSMAUTH_INS_GET_MGMKEY_RETRIES 0x09

// P1 bytes
#define YKHSMAUTH_P1_RESET 0xde

// P2 bytes
#define YKHSMAUTH_P2_RESET 0xad

// Tag codes
#define YKHSMAUTH_TAG_LABEL 0x71
#define YKHSMAUTH_TAG_LABEL_LIST 0x72
#define YKHSMAUTH_TAG_PW 0x73
#define YKHSMAUTH_TAG_ALGO 0x74
#define YKHSMAUTH_TAG_KEY_ENC 0x75
#define YKHSMAUTH_TAG_KEY_MAC 0x76
#define YKHSMAUTH_TAG_CONTEXT 0x77
#define YKHSMAUTH_TAG_RESPONSE 0x78
#define YKHSMAUTH_TAG_VERSION 0x79
#define YKHSMAUTH_TAG_TOUCH 0x7a
#define YKHSMAUTH_TAG_MGMKEY 0x7b

// Algos
#define YKHSMAUTH_YUBICO_AES128_ALGO 38
#define YKHSMAUTH_YUBICO_ECP256_ALGO 39

#define SW_SUCCESS 0x9000
#define SW_ERR_AUTHENTICATION_FAILED 0x63c0
#define SW_FILE_FULL 0x6a84
#define SW_FILE_NOT_FOUND 0x6a82
#define SW_MEMORY_ERROR 0x6581
#define SW_SECURITY_STATUS_NOT_SATISFIED 0x6982

// Lengths
#define YKHSMAUTH_MIN_LABEL_LEN 1
#define YKHSMAUTH_MAX_LABEL_LEN 64
#define YKHSMAUTH_SESSION_KEY_LEN 16
#define YKHSMAUTH_YUBICO_AES128_KEY_LEN 32
#define YKHSMAUTH_PW_LEN 16
#define YKHSMAUTH_CONTEXT_LEN 16

// PBKDF2 derivation parameters
#define YKHSMAUTH_DEFAULT_SALT "Yubico"
#define YKHSMAUTH_DEFAULT_ITERS 10000

typedef struct ykhsmauth_state ykhsmauth_state;

typedef enum {
  YKHSMAUTHR_SUCCESS = 0,
  YKHSMAUTHR_MEMORY_ERROR = -1,
  YKHSMAUTHR_PCSC_ERROR = -2,
  YKHSMAUTHR_GENERIC_ERROR = -3,
  YKHSMAUTHR_WRONG_PW = -4,
  YKHSMAUTHR_INVALID_PARAMS = -5,
  YKHSMAUTHR_ENTRY_NOT_FOUND = -6,
  YKHSMAUTHR_STORAGE_FULL = -7,
  YKHSMAUTHR_TOUCH_ERROR = -8,
} ykhsmauth_rc;

typedef struct {
  uint8_t algo;
  uint8_t touch;
  char label[YKHSMAUTH_MAX_LABEL_LEN + 1];
  uint8_t ctr;
} ykhsmauth_list_entry;

const char *ykhsmauth_strerror(ykhsmauth_rc err);
const char *ykhsmauth_strerror_name(ykhsmauth_rc err);

ykhsmauth_rc ykhsmauth_init(ykhsmauth_state **state, int verbose);
ykhsmauth_rc ykhsmauth_done(ykhsmauth_state *state);
ykhsmauth_rc ykhsmauth_connect(ykhsmauth_state *state, const char *wanted);
ykhsmauth_rc ykhsmauth_list_readers(ykhsmauth_state *state, char *readers,
                                    size_t *len);
ykhsmauth_rc ykhsmauth_disconnect(ykhsmauth_state *state);

ykhsmauth_rc ykhsmauth_get_version(ykhsmauth_state *state, char *version,
                                   size_t len);

ykhsmauth_rc ykhsmauth_put(ykhsmauth_state *state, const uint8_t *mgmkey,
                           size_t mgmkey_len, const char *label, uint8_t algo,
                           const uint8_t *key, size_t key_len,
                           const uint8_t *pw, size_t pw_len,
                           const uint8_t touch_policy, uint8_t *retries);

ykhsmauth_rc ykhsmauth_delete(ykhsmauth_state *state, uint8_t *mgmkey,
                              size_t mgmkey_len, char *label, uint8_t *retries);
ykhsmauth_rc ykhsmauth_calculate(ykhsmauth_state *state, const char *label,
                                 uint8_t *context, size_t context_len,
                                 const uint8_t *pw, size_t pw_len,
                                 uint8_t *key_s_enc, size_t key_s_enc_len,
                                 uint8_t *key_s_mac, size_t key_s_mac_len,
                                 uint8_t *key_s_rmac, size_t key_s_rmac_len,
                                 uint8_t *retries);
ykhsmauth_rc ykhsmauth_reset(ykhsmauth_state *state);
ykhsmauth_rc ykhsmauth_list_keys(ykhsmauth_state *state,
                                 ykhsmauth_list_entry *list,
                                 size_t *list_items);
ykhsmauth_rc ykhsmauth_get_challenge(ykhsmauth_state *state);
ykhsmauth_rc ykhsmauth_get_mgmkey_retries(ykhsmauth_state *state,
                                          uint8_t *retries);
ykhsmauth_rc ykhsmauth_put_mgmkey(ykhsmauth_state *state, uint8_t *mgmkey,
                                  size_t mgmkey_len, uint8_t *new_mgmkey,
                                  size_t new_mgmkey_len, uint8_t *retries);

#ifdef __cplusplus
}
#endif

#endif
