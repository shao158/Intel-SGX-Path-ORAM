#include "PathORAM.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <random>
#include <sys/time.h>

#include "App.h"
#include "Enclave_u.h"

static uint32_t GenerateMyRand() {
  static std::random_device rd;
  static std::mt19937 eng(rd());
  static std::uniform_int_distribution<uint32_t> distr;
  return distr(eng);
}

static uint32_t ComputeTreeHeight(uint64_t cap) {
  uint32_t h = 0;
  while (cap) {
    h += 1;
    cap /= 2;
  }
  // PathORAM paper indicates that (h - 1) is enough for the tree height.
  return h;
}

static uint64_t GetTimeInMicroSec() {
  static struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 * 1000 + tv.tv_usec;
}

uint32_t PathORAM::next_ORAM_id_ {1}; // Starting ORAM id is 1.

PathORAM::PathORAM(uint64_t capacity, uint64_t block_size)
    : ORAM("This is not a file-based ORAM.", block_size) {
  capacity_ = capacity; 
  count_read_ = 0;
  count_write_ = 0;

  my_ORAM_id_ = next_ORAM_id_++;

  // Initialize for pos_map_.
  pos_map_ = nullptr;
  pos_map_capacity_ = capacity_;
  if (pos_map_capacity_ > MAX_POS_MAP) {
    uint64_t num_paths_per_block = block_size_ / sizeof(BlockPath);
    assert(block_size_ % sizeof(BlockPath) == 0 && num_paths_per_block > 1);
    uint64_t higher_capacity =
        (capacity_ + num_paths_per_block - 1)
        / num_paths_per_block;
    
    // Use the same size for this recursive ORAM.
    // Using different choices of block size can refer to the PathORAM paper.
    pos_map_ = new PathORAM(higher_capacity, block_size_);
  }

  // Initialize for enclave client only once.
  if (my_ORAM_id_ == 1) {
    ecall_init_client(global_eid, ComputeTreeHeight(capacity_), block_size_);
  }

  // Initialize for tree_.
  InitializeMemTree(&tree_, ComputeTreeHeight(capacity_), block_size_);

  pos_map_block_ = (uint8_t *)malloc(block_size_);
  fetched_block_ = (uint8_t *)malloc(block_size_);

  std::cout << "Success: Finish init PathORAM (" << capacity << ","
            << block_size_ << ")." << std::endl;
}

PathORAM::~PathORAM() {
  if (my_ORAM_id_ == 1) ecall_destory_client(global_eid);
  if (pos_map_capacity_ > MAX_POS_MAP) delete pos_map_;
  FreeMemTree(&tree_);
  free(pos_map_block_);
  free(fetched_block_);
}

bool PathORAM::write(uint64_t ind, uint8_t* block,
                       uint64_t write_size, uint64_t write_offset) {
  assert(block != nullptr);
  return AccessPathORAM(ind, block, write_size, write_offset);
}

bool PathORAM::read(uint64_t ind, uint8_t* block) {
  if (block != nullptr) free(block);
  block = (uint8_t *)malloc(block_size_);
  return AccessPathORAM(ind, block, 0, 0);
}

bool PathORAM::AccessPathORAM(
    uint64_t ind, uint8_t* block, uint64_t write_size, uint64_t write_offset) {
  assert(ind < pos_map_capacity_);
  /* If key does not exist in position map,
   * the returned block_key will be zero.
   * Since we require any block key in the tree cannot be zero,
   * the returned block will be zero. 
   * Finally we write the zero block back to the tree. */

#ifdef DEBUG_ORAM_ACCESS
  std::cout << "Start accessing ORAM..." << std::endl;
#endif

  uint64_t t0 = GetTimeInMicroSec();

  BlockPath *block_path = new BlockPath();

  do {
    *block_path = GenerateMyRand();
  } while (*block_path == 0);

  const BlockPath new_block_path = *block_path;

  BlockKey block_key = ind + 1;
  const uint64_t num_paths_per_block =  block_size_ / sizeof(BlockPath);
  const uint64_t block_index = ind / num_paths_per_block;
  const uint64_t block_offset = (ind % num_paths_per_block) * sizeof(BlockPath);
  // Two things done here: 1) Updatet the path in pos map. 2) Read the path.
  if (pos_map_capacity_ <= MAX_POS_MAP) {
    ecall_write_mm_in_client(global_eid, ind, block_path, 1);
  } else {
    pos_map_->write(
        block_index, (uint8_t *)block_path, sizeof(BlockPath), block_offset);
  }

  // If the path read is 0, it means the block is not available.
  while (*block_path == 0) {
    *block_path = GenerateMyRand();
  }

  uint64_t t1 = GetTimeInMicroSec();

#ifdef DEBUG_ORAM_ACCESS
  std::cout << "Get and update position map." << std::endl;
#endif

  uint8_t **encrypted_content_ptr = (uint8_t **)calloc(1, sizeof(uint8_t *));
  MemTreeFetch_PathORAM(&tree_, *block_path, block_key,
      fetched_block_, encrypted_content_ptr);

  uint64_t t2 = GetTimeInMicroSec();

  // Failed to fetch the block. Allocate an encrypted space for the new block.
  /*
  if ((uint64_t)(*encrypted_content_ptr) == 0) {
    *encrypted_content_ptr = (uint8_t *)calloc(
          block_size_ + SGX_AESGCM_MAC_SIZE + SGX_AESGCM_IV_SIZE,
          sizeof(uint8_t));
  }
  */

#ifdef DEBUG_ORAM_ACCESS
  std::cout << "Get fetched block." << std::endl;
#endif

  // TODO: Make this oblivious.
  if (write_size > 0) {
    memcpy(pos_map_block_, fetched_block_ + write_offset, write_size);
    memcpy(fetched_block_ + write_offset, block, write_size);
    memcpy(block, pos_map_block_, write_size);
  } else {
    memcpy(pos_map_block_, fetched_block_, block_size_); // Fake operation.
    memcpy(block, fetched_block_, block_size_);
    memcpy(pos_map_block_, fetched_block_, block_size_); // Fake operation.
  }

  uint64_t t3 = GetTimeInMicroSec();

#ifdef DEBUG_ORAM_ACCESS
  std::cout << "Write fetched block." << std::endl;
  std::cout << "Before put: " << (uint64_t)(*ptr_holder) << std::endl;
#endif

  uint32_t stash_load;
  MemTreePut_PathORAM(
      &tree_, new_block_path, block_key,
      fetched_block_, encrypted_content_ptr, &stash_load);
#ifdef DEBUG_ORAM_ACCESS
  std::cout << "Stash load: " << stash_load << std::endl;
#endif

  // if ((uint64_t)(*ptr_holder) != 0) {
    // free(*ptr_holder);
  // }
  free(encrypted_content_ptr);

  uint64_t t4 = GetTimeInMicroSec();

#ifdef DEBUG_ORAM_ACCESS
  std::cout << "After put: " << (uint64_t)(*encrypted_content_ptr) << std::endl;
  std::cout << "Put fetched block." << std::endl;
#endif

  MemTreeFlush_PathORAM(&tree_, *block_path);

  uint64_t t5 = GetTimeInMicroSec();

#ifdef DEBUG_ORAM_ACCESS
  std::cout << "Flush the tree." << std::endl;
#endif

  if (true && my_ORAM_id_ == 1) {
  std::cout << "time 1: " << t1 - t0 << std::endl
            << "time 2: " << t2 - t1 << std::endl  
            << "time 3: " << t3 - t2 << std::endl
            << "time 4: " << t4 - t3 << std::endl
            << "time 5: " << t5 - t4 << std::endl;
  }
  
  return true;
}
