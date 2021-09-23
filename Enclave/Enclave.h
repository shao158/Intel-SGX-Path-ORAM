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

#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <assert.h>
#include <stdlib.h>

#include "Simple9.h"

#include "Conf.h"

#if defined(__cplusplus)
extern "C" {
#endif

int printf_fmt(const char* fmt, ...);

void sleep_enclave(uint64_t time_ms);

void test_ocopy();

void test_omove();

void test_omove_32();

void ObliviousSwap(int64_t cond, int64_t *src, int64_t *dest);

void ObliviousMove32(int32_t cond, int32_t src, int32_t *dest);

void ObliviousPick32(int64_t ind, int32_t *src, int32_t *dest, int64_t len);

void ObliviousPickAndSwap32(int64_t ind, int32_t *src, int32_t *dest, int64_t len);

bool ObliviousNodeRead(MemTreeNode *client_path, uint8_t **client_path_stash[],
                       const uint32_t depth, const BlockKey block_key,
                       const uint32_t block_size, uint8_t *fetched_block,
                       uint8_t **encrypted_content_ptr);

bool ObliviousNodeWrite(MemTreeNode *client_path, uint8_t **client_path_stash[],
                        const uint32_t depth, uint8_t **encrypted_content_ptr,
                        const BlockKey block_key, const BlockPath block_path,
                        const uint32_t block_size, uint8_t *block_content);

bool ObliviousNodeTransfer(MemTreeNode *client_temp_node,
                           uint8_t *client_temp_block_content[],
                           uint32_t block_key, uint32_t block_path,
                           uint8_t *encrypted_content_ptr,
                           uint8_t *block_content,
                           bool input_sign, uint32_t block_size);

void decryptData(uint8_t *encDataIn, uint32_t len,
                 uint8_t *decDataOut, uint32_t len_out);

void encryptData(uint8_t *decDataIn, uint32_t len,
                 uint8_t *encDataOut, uint32_t len_out);

#if defined(__cplusplus)
}
#endif

#endif /* !_ENCLAVE_H_ */
