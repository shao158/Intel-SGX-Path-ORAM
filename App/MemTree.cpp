#include <stdio.h>
#include <inttypes.h>
#include <iostream>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <random>

#include "App.h"
#include "Conf.h"
#include "Enclave_u.h"

#include "MemTree.h"

bool InitializeMemTree(MemTree *tree, uint32_t height, uint32_t block_size) {
  // Assume block_size is a multiple of 8,
  // so we can do ocopy per uint64_t.
  assert(block_size % 8 == 0);

  tree->height = height;
  tree->block_size = block_size;
  uint64_t capacity = 1LL << height;
#ifdef AES_ENC
  tree->nodes = (EncMemTreeNode*) calloc(capacity, sizeof(EncMemTreeNode));
  MemTreeNode empty_node;
  memset(&empty_node, 0, sizeof(MemTreeNode));
  
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;
  for (uint64_t i = 0; i < (1LL << height); i++) {
    for (uint32_t i = 0; i < BLOCKS_PER_NODE; i++) {
      // This memory space is for encrypted ORAM blocks.
      empty_node.block_contents[i] =
          (uint8_t *)calloc(
              block_size + SGX_AESGCM_MAC_SIZE + SGX_AESGCM_IV_SIZE,
              sizeof(uint8_t));
    }

    EncMemTreeNode *enc_node = &tree->nodes[i];
    ret = ecall_encryptData(global_eid,
        (uint8_t *)&empty_node, sizeof(MemTreeNode),
        (uint8_t *)enc_node, sizeof(EncMemTreeNode));
    assert(ret == SGX_SUCCESS);
    /*
    assert(EncryptDoc((const byte*)&empty_node, sizeof(MemTreeNode),
           (const byte*)tree->aes_key.key, (byte*)enc_node->iv,
           (byte*)enc_node->ciphertext) == sizeof(enc_node->ciphertext));
    */
  }
#else
  tree->nodes = (MemTreeNode*) calloc(capacity, sizeof(MemTreeNode));
  if (tree->nodes == NULL) return false;
  for (uint64_t i = 0; i < capacity; i++) {
    for (uint64_t j = 0; j < BLOCKS_PER_NODE; j++) {
      tree->nodes[i].block_contents[j] =
          (uint8_t *) calloc(block_size, sizeof(uint8_t));
    }
  }
#endif

  return true;
}


bool FreeMemTree(MemTree *tree) {
  uint64_t capacity = 1LL << (tree->height);
#ifdef AES_ENC
  MemTreeNode empty_node;
  memset(&empty_node, 0, sizeof(MemTreeNode));
  
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;
  for (uint64_t i = 0; i < (1LL << tree->height); i++) {
    EncMemTreeNode *enc_node = &tree->nodes[i];
    ret = ecall_decryptData(global_eid,
        (uint8_t *)enc_node, sizeof(EncMemTreeNode),
        (uint8_t *)&empty_node, sizeof(MemTreeNode));
    assert(ret == SGX_SUCCESS);

    for (uint32_t i = 0; i < BLOCKS_PER_NODE; i++) {
      free(empty_node.block_contents[i]);
    }
  }
#else
  for (uint64_t i = 0; i < capacity; i++) {
    for (uint64_t j = 0; j < BLOCKS_PER_NODE; j++) {
      free(tree->nodes[i].block_contents[j]);
    }
  }
#endif
  free(tree->nodes);
  return true;
}

void MemTreeFetch(MemTree *tree, BlockPath block_path,
                  const BlockKey block_key, uint8_t *fetched_block) {
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;

#ifdef AES_ENC
  ret = ecall_download_path_to_enclave_client_aes(global_eid,
#else
  ret = ecall_download_path_to_enclave_client(global_eid,
#endif
      tree->nodes, tree->height, block_path, tree->block_size);
  assert(ret == SGX_SUCCESS);

  ret = ecall_oblivious_fetch(global_eid,
      tree->height, tree->block_size, block_key, fetched_block, nullptr);
  assert(ret == SGX_SUCCESS);

#ifdef AES_ENC
  ret = ecall_update_path_outside_enclave_aes(global_eid,
#else
  ret = ecall_update_path_outside_enclave(global_eid,
#endif
      tree->nodes, tree->height, block_path, tree->block_size);
  assert(ret == SGX_SUCCESS);
}

void MemTreePut(MemTree *tree,
                const BlockPath block_path, const BlockKey block_key,
                uint8_t* block_content, uint32_t *root_load) {
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;
#ifdef AES_ENC
  ret = ecall_oblivious_put_to_root_aes(global_eid, tree->nodes, tree->height,
#else
  ret = ecall_oblivious_put_to_root(global_eid, tree->nodes, tree->height,
#endif
                                    tree->block_size, block_path, block_key,
                                    block_content, root_load);
  assert(ret == SGX_SUCCESS);
}

void MemTreeFlush(MemTree *tree, BlockPath rand_path) {
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;

#ifdef AES_ENC
  ret = ecall_download_path_to_enclave_client_aes(global_eid,
#else
  ret = ecall_download_path_to_enclave_client(global_eid,
#endif
      tree->nodes, tree->height, rand_path, tree->block_size);
  assert(ret == SGX_SUCCESS);

  ret = ecall_client_simple_oram_flush(global_eid,
      tree->height, tree->block_size, rand_path);
  assert(ret == SGX_SUCCESS);

#ifdef AES_ENC
  ret = ecall_update_path_outside_enclave_aes(global_eid,
#else
  ret = ecall_update_path_outside_enclave(global_eid,
#endif
      tree->nodes, tree->height, rand_path, tree->block_size);
  assert(ret == SGX_SUCCESS);
}

#ifdef AES_ENC
void MemTreeFetch_PathORAM(MemTree *tree,
                           BlockPath block_path,
                           const BlockKey block_key,
                           uint8_t *fetched_block,
                           uint8_t **encrypted_content_ptr) {
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;

  ret = ecall_download_path_to_enclave_client_path_oram(global_eid,
      tree->nodes, tree->height, block_path, tree->block_size);
  assert(ret == SGX_SUCCESS);

  ret = ecall_oblivious_fetch(global_eid,
      tree->height, tree->block_size, block_key, fetched_block,
      encrypted_content_ptr);
  assert(ret == SGX_SUCCESS);
}

void MemTreePut_PathORAM(MemTree *tree,
                         const BlockPath block_path, const BlockKey block_key,
                         uint8_t* block_content,
                         uint8_t **encrypted_content_ptr,
                         uint32_t *root_load) {
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;
  ret = ecall_oblivious_put_to_stash_path_oram(
      global_eid, tree->nodes, tree->height,
      tree->block_size, block_path, block_key, encrypted_content_ptr,
      block_content, root_load);
  assert(ret == SGX_SUCCESS);
}

void MemTreeFlush_PathORAM(MemTree *tree, BlockPath rand_path) {
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;
  ret = ecall_evict_path_outside_enclave_path_oram(global_eid,
      tree->nodes, tree->height, rand_path, tree->block_size);
  assert(ret == SGX_SUCCESS);
}
#endif
