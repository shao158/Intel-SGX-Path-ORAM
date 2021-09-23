#ifndef _ORAM_H_
#define _ORAM_H_

#include <cstdint>
#include <string>

class ORAM {
protected:
  const std::string oram_file_name_;
  uint64_t capacity_;
  uint64_t block_size_;

  uint64_t count_read_;
  uint64_t count_write_;

public:
  ORAM(const char* oram_file_name, uint64_t block_size) 
    : oram_file_name_(oram_file_name), 
      block_size_(block_size) {}

  virtual ~ORAM() = default;

  ORAM(const ORAM&) = delete;
  ORAM& operator=(const ORAM&) = delete;

  inline uint64_t getCapacity() const { return capacity_; }
  inline uint64_t getBlockSize() const { return block_size_; }

  inline uint64_t getCountRead() const { return count_read_; }
  inline uint64_t getCountWrite() const { return count_write_; }

  // ind is the block id.
  // block should be not be NULL, and has write_size bytes.
  virtual bool write(uint64_t ind, uint8_t* block,
                     uint64_t write_size, uint64_t write_offset) = 0;

  // ind is the block id.
  // block will be freed and re-allocated with size of block_size_.
  virtual bool read(uint64_t ind, uint8_t* block) = 0;
};

#endif // _ORAM_H
