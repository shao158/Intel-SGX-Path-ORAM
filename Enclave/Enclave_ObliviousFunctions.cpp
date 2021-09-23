#include "Enclave.h"
#include "Enclave_t.h"

#include <cstdlib>

#include "Conf.h"

// When cond is 0, *dest = val_1; Otheriwse *dest = val_2;
void omove(int64_t cond, int64_t val_1, int64_t val_2, int64_t *dest) {
  __asm__ (
      "movq %2, %%rdx;"
      "testq %0, %0;"
      "cmovzq %1, %%rdx;"
      "movq %%rdx, (%[dest]);"
      :
      : [in] "r" (cond),  "r" (val_1), "r" (val_2),
        [dest] "r" (dest)
      : "cc", "%rdx"
  );
}

// When cond is 0, *dest = val_1; Otheriwse *dest = val_2;
void omove_32(int32_t cond, int32_t val_1, int32_t val_2, int32_t *dest) {
  __asm__ (
      "movl %2, %%edx;"
      "test %0, %0;"
      "cmovz %1, %%edx;"
      "movl %%edx, (%[dest]);"
      :
      : [in] "r" (cond),  "r" (val_1), "r" (val_2),
        [dest] "r" (dest)
      : "cc", "%edx"
  );
}

// When cond is 1, do the copy.
void ocopy(int64_t cond, int64_t* src, int64_t* dest, int64_t len) {
  int64_t count = 0;
  __asm__ (
      "1:;"
      "movq (%[src], %2, 8), %%r12;"
      "movq (%[dest], %2, 8), %%r13;"
      "testq %0, %0;"
      "cmovnzq %%r12, %%r13;"
      "movq %%r13, (%[dest], %2, 8);"
      "inc %2;"
      "cmp %1, %2;"
      "jl 1b;"
      :
      : [in] "r" (cond), "r" (len), "r" (count),
        [src] "r" (src),
        [dest] "r" (dest)
      : "cc", "%r12", "%r13"
  );
}

// When cond is 1, swap the value in src and dest.
void oswap(int64_t cond, int64_t *src, int64_t *dest) {
  __asm__ (
      "movq (%[src]), %%r12;"
      "movq (%[dest]), %%r13;"
      "testq %0, %0;"
      "cmovnzq %%r13, %%r14;"
      "cmovnzq %%r12, %%r13;"
      "cmovnzq %%r14, %%r12;"
      "movq %%r12, (%[src]);"
      "movq %%r13, (%[dest]);"
      :
      : [in] "r" (cond),
        [src] "r" (src),
        [dest] "r" (dest)
      : "cc", "%r12", "%r13", "r14"
  );
}

void test_ocopy() {
  int64_t tmp_cond = 0;
  int64_t len = 6;
  int64_t *tmp_src = (int64_t *) malloc(len * sizeof(int64_t));
  int64_t *tmp_dest = (int64_t *) malloc(len * sizeof(int64_t));
  for (int i = 0; i < len; i++) {
    tmp_src[i] = 1LL << (20 + i);
    tmp_dest[i] = 0;
  }
  for (int i = 0; i < len; i++) {
    printf_fmt("Before ocopy, dest: %lld, src: %lld \n", tmp_dest[i], tmp_src[i]);
  }
  ocopy(tmp_cond, tmp_src, tmp_dest, len);
  for (int i = 0; i < len; i++) {
    printf_fmt("After ocopy, dest: %lld\n", tmp_dest[0]);
  }

  tmp_cond = 1;

  for (int i = 0; i < len; i++) {
    tmp_src[i] = 1LL << (30 + i);
    tmp_dest[i] = 0;
  }
  for (int i = 0; i < len; i++) {
    printf_fmt("Before ocopy, dest: %lld, src: %lld \n", tmp_dest[i], tmp_src[i]);
  }
  for (int i = 0; i < 1000 * 1000; i++) {
    ocopy(tmp_cond, tmp_src, tmp_dest, len);
  }
  for (int i = 0; i < len; i++) {
    printf_fmt("After ocopy, dest: %lld\n", tmp_dest[i]);
  }

  free(tmp_src);
  free(tmp_dest);
}

void test_omove() {
  int64_t tmp_cond = 0, val1 = 1LL << 40, val2 = 1LL << 50;
  int64_t *tmp_dest = (int64_t *) malloc(sizeof(int64_t));
  *tmp_dest = 0;
  printf_fmt("Before omove, dest: %lld, val1: %lld, val2: %lld \n",
      *tmp_dest, val1, val2);
  omove(tmp_cond, val1, val2, tmp_dest);
  printf_fmt("After omove, dest: %lld\n", *tmp_dest);

  tmp_cond = 1;
  
  *tmp_dest = 0;

  printf_fmt("Before omove, dest: %lld, val1: %lld, val2: %lld \n",
      *tmp_dest, val1, val2);
  omove(tmp_cond, val1, val2, tmp_dest);
  printf_fmt("After omove, dest: %lld\n", *tmp_dest);

  free(tmp_dest);
}

void test_omove_32() {
  int32_t tmp_cond = 0, val1 = 1LL << 20, val2 = 1LL << 23;
  int32_t *tmp_dest = (int32_t *) malloc(sizeof(int32_t));
  *tmp_dest = 0;
  printf_fmt("Before omove, dest: %d, val1: %d, val2: %d \n",
      *tmp_dest, val1, val2);
  omove_32(tmp_cond, val1, val2, tmp_dest);
  printf_fmt("After omove, dest: %d\n", *tmp_dest);

  tmp_cond = 1;
  
  *tmp_dest = 0;

  printf_fmt("Before omove, dest: %d, val1: %d, val2: %d \n",
      *tmp_dest, val1, val2);
  omove_32(tmp_cond, val1, val2, tmp_dest);
  printf_fmt("After omove, dest: %d\n", *tmp_dest);

  free(tmp_dest);
}

void ObliviousPick32(int64_t ind, int32_t *src, int32_t *dest, int64_t len) {
  int64_t count = 0;
  __asm__ (
      "1:;"
      "mov (%[src], %2, 4), %%r12;"
      "mov (%[dest]), %%r13;"
      "cmp %0, %2;"
      "cmove %%r12, %%r13;"
      "mov %%r13, (%[dest]);"
      "inc %2;"
      "cmp %1, %2;"
      "jl 1b;"
      :
      : [in] "r" (ind), "r" (len), "r" (count),
        [src] "r" (src),
        [dest] "r" (dest)
      : "cc", "%r12", "%r13"
  );
}

void ObliviousPickAndSwap32(
  int64_t ind, int32_t *src, int32_t *dest, int64_t len) {
  int64_t count = 0;
  __asm__ (
      "1:;"
      "mov (%[src], %2, 4), %%r12;"
      "mov (%[dest]), %%r13;"
      "cmp %0, %2;"
      "cmove %%r13, %%r14;"
      "cmove %%r12, %%r13;"
      "cmove %%r14, %%r12;"
      "mov %%r12, (%[src], %2, 4);"
      "mov %%r13, (%[dest]);"
      "inc %2;"
      "cmp %1, %2;"
      "jl 1b;"
      :
      : [in] "r" (ind), "r" (len), "r" (count),
        [src] "r" (src),
        [dest] "r" (dest)
      : "cc", "%r12", "%r13", "%r14"
  );
}

void ObliviousMove32(int32_t cond, int32_t src, int32_t *dest) {
  omove_32(cond, *dest, src, dest);
}

void ObliviousSwap(int64_t cond, int64_t *src, int64_t *dest) {
  oswap(cond, src, dest);
}

/* Read a block from the node on client path with given depth obliviously.
 * Assuming:
 * 1) if block_key is found before, block is correctly valued;
 * 2) if block_key is not found yet, block is zero valued. */
bool ObliviousNodeRead(MemTreeNode *client_path,
                       uint8_t **client_path_stash[],
                       const uint32_t depth, const BlockKey block_key,
                       const uint32_t block_size, uint8_t *fetched_block, 
                       uint8_t **encrypted_content_ptr) {
  register uint32_t reg_num_blocks = client_path[depth].num_blocks;

  bool succ = false;
  for (uint32_t i = 0; i < BLOCKS_PER_NODE; i++) {
    register BlockKey reg_key = client_path[depth].block_keys[i];
    // register BlockPath reg_path = client->path[depth].block_paths[i];
    register uint8_t reg_presence = client_path[depth].presence_bits[i];
    const bool sign = reg_key == block_key && reg_presence > 0;

    reg_key -= (reg_key & sign);
    reg_num_blocks -= (1 & sign);
    reg_presence -= (reg_presence & sign); 

#ifdef PATH_ORAM
    // Make this oblivious.
    if (sign) *encrypted_content_ptr = client_path[depth].block_contents[i];
#endif

    succ = succ || (sign > 0);

    ocopy(sign,
          (int64_t *)(client_path_stash[depth][i]),
          (int64_t *)(fetched_block), block_size / 8);

    client_path[depth].block_keys[i] = reg_key;
    client_path[depth].presence_bits[i] = reg_presence;
    // client->path[depth].block_paths[i] = reg_path;
    client_path[depth].num_blocks = reg_num_blocks;
  }

  return succ;
}

bool ObliviousNodeTransfer(MemTreeNode *client_temp_node,
                           uint8_t *client_temp_block_content[],
                           uint32_t block_key, uint32_t block_path,
                           uint8_t *encrypted_content_ptr,
                           uint8_t *block_content,
                           bool input_sign, uint32_t block_size) {
  BlockKey mask = 0;
  mask = -(input_sign == false);

  for (uint32_t i = 0; i < BLOCKS_PER_NODE; i++) {
    const BlockKey sign = -(client_temp_node->presence_bits[i] == 0);
    const BlockKey masked_sign = (sign & (~mask));

    client_temp_node->block_keys[i] = (block_key & masked_sign) |
        (client_temp_node->block_keys[i] & (~masked_sign));
    client_temp_node->block_paths[i] = (block_path & masked_sign) |
        (client_temp_node->block_paths[i] & (~masked_sign));
    client_temp_node->presence_bits[i] = (1 & masked_sign) |
        (client_temp_node->presence_bits[i] & (~masked_sign));
#ifdef PATH_ORAM
    if (masked_sign)
      client_temp_node->block_contents[i] = encrypted_content_ptr;
#endif
    ocopy(masked_sign,
          (int64_t *)(block_content),
          (int64_t *)(client_temp_block_content[i]), block_size / 8);
    client_temp_node->num_blocks += (masked_sign != 0);
    mask |= masked_sign;
  }

  return input_sign;
}

/* Write a block into the node on client path with given depth obliviously. */
bool ObliviousNodeWrite(MemTreeNode *client_path, uint8_t **client_path_stash[],
                        const uint32_t depth, uint8_t **encrypted_content_ptr,
                        const BlockKey block_key, const BlockPath block_path,
                        const uint32_t block_size, uint8_t *block_content) {
  // printf_fmt("Take a node at depth %d.\n", depth);
  // if ((client_path[depth].num_blocks >= BLOCKS_PER_NODE)) return false;
  // if (block_key == 0) return false;
  BlockKey mask = 0;
  bool is_failed = (block_key == 0)
      || (client_path[depth].num_blocks >= BLOCKS_PER_NODE);

  mask = -(is_failed ? 1 : 0);

  for (uint32_t i = 0; i < BLOCKS_PER_NODE; i++) {
    const BlockKey sign = -(client_path[depth].presence_bits[i] == 0);
    const BlockKey masked_sign = (sign & (~mask));
    client_path[depth].block_keys[i] = (block_key & masked_sign) |
        (client_path[depth].block_keys[i] & (~masked_sign));
    client_path[depth].block_paths[i] = (block_path & masked_sign) |
        (client_path[depth].block_paths[i] & (~masked_sign));
    client_path[depth].presence_bits[i] = (1 & masked_sign) |
        (client_path[depth].presence_bits[i] & (~masked_sign));
#ifdef PATH_ORAM
    if (masked_sign) { 
      uint8_t *tmp_ptr_holder = client_path[depth].block_contents[i];
      client_path[depth].block_contents[i] = *encrypted_content_ptr;
      *encrypted_content_ptr = tmp_ptr_holder;
    }
#endif
    ocopy(masked_sign,
          (int64_t *)(block_content),
          (int64_t *)(client_path_stash[depth][i]), block_size / 8);
    mask |= sign;
  }
  client_path[depth].num_blocks += (is_failed ? 0 : 1);
  return !is_failed;
}
