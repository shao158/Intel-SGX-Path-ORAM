#ifndef _PATH_ORAM_H_
#define _PATH_ORAM_H_

#include <cstdint>

#include "ORAM.h"
#include "MemTree.h"

class PathORAM : public ORAM {
private:
  uint64_t pos_map_capacity_;
  PathORAM* pos_map_; // Used only when pos_map_capacity > MAX_POS_MAP.
  MemTree tree_;
  uint8_t *pos_map_block_;
  uint8_t *fetched_block_;
  bool AccessPathORAM(
      uint64_t ind, uint8_t* block, uint64_t write_size, uint64_t write_offset);

  static uint32_t next_ORAM_id_;
  uint32_t my_ORAM_id_;

public:
  // If not init successfuly, capacity_ in ORAM.h would be still 0.
  explicit PathORAM(uint64_t capacity, uint64_t block_size);

  ~PathORAM();

  PathORAM(const PathORAM&) = delete;
  PathORAM& operator=(const PathORAM&) = delete;

  // Write the content of block into ORAM.
  // And load the old content into block.
  // ind is the block id.
  // block should be not be NULL, and has write_size bytes.
  // Caller is responsible for allocating and freeing block.
  bool write(uint64_t ind, uint8_t* block,
             uint64_t write_size, uint64_t write_offset);

  // ind is the block id.
  // block will be freed and re-allocated with size of block_size_.
  // Caller is responsible for allocating and freeing block.
  bool read(uint64_t ind, uint8_t* block);
};

#endif
