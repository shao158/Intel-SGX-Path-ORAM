#ifndef _MEM_TREE_H_
#define _MEM_TREE_H_

#include <cstdint>

#include "Conf.h"

/* ORAM tree should be put on server side. */
typedef struct MemTree {

  /* Height of the tree. */
  uint32_t height;

  /* Number of bytes used in each block. */
  uint32_t block_size;

  /* Node contents. */
  #ifdef AES_ENC
  EncMemTreeNode *nodes;
  #else
  MemTreeNode *nodes;
  #endif

} MemTree;

bool InitializeMemTree(MemTree *tree, uint32_t height, uint32_t block_size);

bool FreeMemTree(MemTree *tree);

/* Fetch the block with the block key.
 * If block_key does not exist, return a zero block. */
void MemTreeFetch(MemTree *tree,
                  BlockPath block_path,
                  const BlockKey block_key,
                  uint8_t *fetched_block);

void MemTreeFetch_PathORAM(MemTree *tree,
                           BlockPath block_path,
                           const BlockKey block_key,
                           uint8_t *fetched_block,
                           uint8_t **encrypted_content_ptr);

/* Put block content to the root.
 * Update root_load as the number of blocks in root node.
 * Return false upon overflow or failure. */
void MemTreePut(MemTree *tree,
                const BlockPath block_path, const BlockKey block_key,
                uint8_t* block_content, uint32_t *root_load);

void MemTreePut_PathORAM(MemTree *tree,
                         const BlockPath block_path, const BlockKey block_key,
                         uint8_t* block_content,
                         uint8_t **encrypted_content_ptr,
                         uint32_t *root_load);

/* Flush blocks along a path chosen at random. */
void MemTreeFlush(MemTree *tree, BlockPath rand_path);

void MemTreeFlush_PathORAM(MemTree *tree, BlockPath rand_path);

#endif // _MEM_TREE_H_
