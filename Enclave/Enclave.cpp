/*
 * Copyright (C) 2011-2020 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "Enclave.h"

#include "sgx_trts.h"
#include "sgx_tcrypto.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h> /* vsnprintf */
#include <string.h>

#include "Enclave_t.h" /* print_string */

/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
int printf_fmt(const char* fmt, ...)
{
    char buf[BUFSIZ] = { '\0' };
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
    return (int)strnlen(buf, BUFSIZ - 1) + 1;
}

void sleep_enclave(uint64_t time_ms)
{
  ocall_sleep(time_ms);
}

#define ENC_BUF_LEN 2048
static sgx_aes_gcm_128bit_key_t enclave_data_key =
    { 0x0, 0x1, 0x2, 0x3,
      0x4, 0x5, 0x6, 0x7,
      0x8, 0x9, 0xa, 0xb,
      0xc, 0xd, 0xe, 0xf };

void decryptData(uint8_t *encDataIn, uint32_t len,
                 uint8_t *decDataOut, uint32_t len_out) {
  uint8_t p_dest[ENC_BUF_LEN] = {0};
  
  sgx_rijndael128GCM_decrypt(
      &enclave_data_key,
      encDataIn + SGX_AESGCM_MAC_SIZE + SGX_AESGCM_IV_SIZE,
      len_out,
      p_dest,
      encDataIn + SGX_AESGCM_MAC_SIZE,
      SGX_AESGCM_IV_SIZE,
      NULL,
      0,
      (sgx_aes_gcm_128bit_tag_t *) encDataIn);
  memcpy(decDataOut, p_dest, len_out);
}

void encryptData(uint8_t *decDataIn, uint32_t len,
                 uint8_t *encDataOut, uint32_t len_out) {
  uint8_t p_dest[ENC_BUF_LEN] = {0};
  
  sgx_read_rand(p_dest + SGX_AESGCM_MAC_SIZE, SGX_AESGCM_IV_SIZE);
  
  sgx_rijndael128GCM_encrypt(
      &enclave_data_key,
      decDataIn,
      len,
      p_dest + SGX_AESGCM_MAC_SIZE + SGX_AESGCM_IV_SIZE,
      p_dest + SGX_AESGCM_MAC_SIZE,
      SGX_AESGCM_IV_SIZE,
      NULL,
      0,
      (sgx_aes_gcm_128bit_tag_t *) p_dest);
  memcpy(encDataOut, p_dest, len_out);
}

void ecall_decryptData(uint8_t *encDataIn, uint32_t len,
                       uint8_t *decDataOut, uint32_t len_out) {
  decryptData(encDataIn, len, decDataOut, len_out);
}

void ecall_encryptData(uint8_t *decDataIn, uint32_t len,
                       uint8_t *encDataOut, uint32_t len_out) {
  encryptData(decDataIn, len, encDataOut, len_out);
}

void ecall_test_aes() {
  return;
}

void ecall_enclaveString(char *s, size_t len) {
  const char *secret = "Hello Enclave!";
  if (len > strlen(secret)) {
    memcpy(s, secret, strlen(secret) + 1);
  } else {
    memcpy(s, "false", strlen("false") + 1);
  }
}

SGX_FILE *ecall_file_open(const char *filename, const char *mode) {
  SGX_FILE *a;
  a = sgx_fopen_auto_key(filename, mode);
  return a;
}

uint64_t ecall_file_get_file_size(SGX_FILE *fp) {
  uint64_t file_size = 0;
  sgx_fseek(fp, 0, SEEK_END);
  file_size = sgx_ftell(fp);
  return file_size;
}

size_t ecall_file_write(SGX_FILE *fp, uint8_t *data, uint64_t len) {
  size_t sizeofWrite = 0;
  sizeofWrite = sgx_fwrite(data, sizeof(uint8_t), len, fp);
  return sizeofWrite;
}

size_t ecall_file_read(SGX_FILE *fp, uint8_t *readData, uint64_t size) {
  sgx_fseek(fp, 0, SEEK_END);
  uint64_t file_length = sgx_ftell(fp);
  sgx_fseek(fp, 0, SEEK_SET);

  char *data;
  data = (char *)malloc(file_length * sizeof(char));
  size_t sizeofRead = sgx_fread(data, sizeof(char), file_length, fp);

  for (size_t i = 0; i < file_length; i++) readData[i] = (uint8_t)data[i];
  free(data);

  return sizeofRead;
}

int32_t ecall_file_close(SGX_FILE *fp) {
  int32_t a;
  a = sgx_fclose(fp);
  return a;
}

void ecall_test_osort(uint32_t len, uint32_t do_sort) {
  /*
  uint64_t *data = (uint64_t *)malloc(len * sizeof(uint64_t));
  sgx_read_rand((uint8_t *)data, len * 8);
  // for (int i = 0; i < len; i++) printf_fmt("%lu ", data[i]);
  // printf_fmt("\n");

  uint32_t count_swap = 0;

  if (do_sort)
  for (uint32_t k = 2; k <= len; k *= 2) for (uint32_t j = k / 2; j > 0; j /= 2)
    for (uint32_t i = 0; i < len; i++) {
      uint32_t l = i ^ j;
      if (l > i) {
      count_swap += 1;
      ObliviousSwap(
          (l > i) && (((i & k) == 0 && data[i] > data[l])
                      || ((i & k) != 0 && data[i] < data[l])),
          (int64_t *) data + i, (int64_t *) data + l);
      }
    }
  // for (int i = 0; i < len; i++) printf_fmt("%lu ", data[i]);
  // printf_fmt("\n");
  printf_fmt("%d\n", count_swap);
  free(data);
  */
  test_ocopy();
}
