#ifndef _CONF_H
#define _CONF_H

#include <stdint.h>

#define SGX_AESGCM_MAC_SIZE 16
#define SGX_AESGCM_IV_SIZE 12

// #define BYTES_PER_BLOCK 1024
#define CLIENT_MAX_PATH 20
// #define POS_BLOCK_SIZE 1024

typedef uint32_t BlockKey;
typedef uint32_t BlockPath;

// Z: 5 for PathORAM, 10 for SimpleORAM.
// Make sure BLOCKS_PER_NODE is smaller than 32.
// Otherwise, change the presence_bits field in MemTreeNode.
#define BLOCKS_PER_NODE 4
#define FLUSH_THRESH_ROOT 1
#define MAX_POS_MAP 65536

#define AES_ENC
#define PATH_ORAM
//#define FILE_ORAM
//#define CIRCUIT_ORAM
//#define DEBUG_ORAM_ACCESS

typedef struct MemTreeNode {

  /* Number of blocks under this node/directory. */
  uint32_t num_blocks;

  /* Path info of the blocks under this node/directory. */
  BlockPath block_paths[BLOCKS_PER_NODE];

  /* Names of the blocks under this node/directory.
   * Have to use 64 bit to handle million keys. */
  BlockKey block_keys[BLOCKS_PER_NODE];

  /* Block content of the blocks under this node/directory. */
  uint8_t *block_contents[BLOCKS_PER_NODE];

  /* The i-th num being 0 indicates that the i-th slot is empty. */
  uint8_t presence_bits[BLOCKS_PER_NODE];

} MemTreeNode;

/* Encrypted tree node. */
typedef struct EncMemTreeNode {
  uint8_t data[sizeof(MemTreeNode) + SGX_AESGCM_MAC_SIZE + SGX_AESGCM_IV_SIZE];
} EncMemTreeNode;

#endif // _CONF_H_
