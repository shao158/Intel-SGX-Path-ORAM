#include "Enclave.h"
#include "Enclave_t.h"

#include "Conf.h"

#include <cstdlib>
#include <cstring>

static void print_path_binary(uint32_t path) {
  uint32_t leading_one = 1;
  for (int i = 31; i >= 0; i--) {
    if (path & (leading_one << i)) {
      printf_fmt("1");
    } else {
      printf_fmt("0");
    }
  }
  printf_fmt("\n");
}

typedef struct Client {

  /* Highest position map.
   * This field is only used for small position map,
   * namely, when capacity is no more than MAX_SMALL_POS_MAP. 
   * See position-map.h for more info. */
  BlockPath path_mm[MAX_POS_MAP];

  /* Data space to load path from server. */
  MemTreeNode *path;

  uint8_t **encrypted_data_addr_in_path[CLIENT_MAX_PATH];

  MemTreeNode *temp_node;

  uint8_t *temp_block_content[BLOCKS_PER_NODE];

  /* Data space to load ORAM blocks in this.path. */
  uint8_t **path_stash[CLIENT_MAX_PATH];

  uint32_t client_heighest_height;

  /* Deepest array for CircuitORAM eviction.
   * Note that -1 denotes that the node is empty (no deepest)! */
  // int deepest[MAX_TREE_HEIGHT];

  /* Target array for CircuitORAM eviction.
   * Note that -1 denotes that the node has no target! */
  // int targets[MAX_TREE_HEIGHT];

  uint32_t stash_load;

  uint8_t *dummy_block;

} Client;

Client *client;

void ecall_read_mm_in_client(uint64_t ind,
                             uint32_t *block_path,
                             uint64_t block_path_size) {
  ObliviousPick32((int64_t)ind,
                  (int32_t *)client->path_mm, (int32_t *)block_path,
                  (int64_t)MAX_POS_MAP);
}

void ecall_write_mm_in_client(uint64_t ind,
                              uint32_t *block_path,
                              uint64_t block_path_size) {
  ObliviousPickAndSwap32((int64_t)ind,
                         (int32_t *)client->path_mm, (int32_t *)block_path,
                         (int64_t)MAX_POS_MAP);
  /*
  uint32_t old_block_path;
  for (uint64_t i = 0; i < MAX_POS_MAP; i++) {
    ObliviousMove32(i == ind, (int32_t)client->path_mm[ind], (int32_t *)&old_block_path);
    ObliviousMove32(i == ind, (int32_t)*block_path, (int32_t *)client->path_mm + ind);
  }
  *block_path = old_block_path;
  */
  /*
  uint64_t old_block_path = client->path_mm[ind];
  client->path_mm[ind] = *block_path;
  *block_path = old_block_path;
  */
}

void ecall_init_client(uint32_t height, uint32_t block_size) {
  // If need more, modify the data type of BlockPath.
  assert(height < CLIENT_MAX_PATH);

  client = (Client *)malloc(1 * sizeof(Client));
  for (int i = 0; i < MAX_POS_MAP; i++) client->path_mm[i] = 0;
  client->path = (MemTreeNode *)calloc(CLIENT_MAX_PATH, sizeof(MemTreeNode));
  for (int i = 0; i < CLIENT_MAX_PATH; i++) {
    client->path_stash[i] =
        (uint8_t **)calloc(BLOCKS_PER_NODE, sizeof(uint8_t *));
    client->encrypted_data_addr_in_path[i] =
        (uint8_t **)calloc(BLOCKS_PER_NODE, sizeof(uint8_t *));
    for (int j = 0; j < BLOCKS_PER_NODE; j++) {
      client->path_stash[i][j] = (uint8_t *)calloc(1, block_size);
    }
  }
  client->client_heighest_height = height;

  client->temp_node = (MemTreeNode *)calloc(1, sizeof(MemTreeNode));
  for (int i = 0; i < BLOCKS_PER_NODE; i++) {
    client->temp_block_content[i] = (uint8_t *)calloc(1, block_size);
  }
  client->stash_load = 0;

  client->dummy_block = (uint8_t *)calloc(block_size, sizeof(uint8_t));
}

void ecall_destory_client() {
  free(client->path);
  for (int i = 0; i < CLIENT_MAX_PATH; i++) {
    for (int j = 0; j < BLOCKS_PER_NODE; j++) {
      free(client->path_stash[i][j]);
    }
    free(client->path_stash[i]);
    free(client->encrypted_data_addr_in_path[i]);
  }
  free(client->dummy_block);
  free(client->temp_node);
  for (int i = 0; i < BLOCKS_PER_NODE; i++) {
    free(client->temp_block_content[i]);
  }
  free(client);
}

void ecall_download_path_to_enclave_client_path_oram(EncMemTreeNode *all_nodes,
                                                     uint32_t height,
                                                     uint32_t block_path,
                                                     uint32_t block_size) {
  // assert(height == client->client_path_height);
  uint32_t addr = 0;
  uint32_t tree_path = (uint32_t) block_path;
  
  for (uint32_t h = 0; h < height; h++) {
    EncMemTreeNode *enc_node = all_nodes + addr;
    decryptData((uint8_t *)enc_node, sizeof(EncMemTreeNode),
                (uint8_t *)client->temp_node, sizeof(MemTreeNode));
    for (uint32_t i = 0; i < BLOCKS_PER_NODE; i++) {
      // Back up the addr of encrypted data.
      client->encrypted_data_addr_in_path[h][i] =
          client->temp_node->block_contents[i];
      decryptData(
          (uint8_t *)(client->temp_node->block_contents[i]),
          block_size + SGX_AESGCM_MAC_SIZE + SGX_AESGCM_IV_SIZE,
          (uint8_t *)(client->temp_block_content[0]),
          block_size);

      bool succ = false;
      for (uint32_t stash_i = 0; stash_i < CLIENT_MAX_PATH; stash_i++) {
        if (succ) {
          ObliviousNodeWrite(client->path, client->path_stash, stash_i,
              client->temp_node->block_contents + i,
              0, // client->temp_node->block_keys[i],
              client->temp_node->block_paths[i],
              block_size,
              client->temp_block_content[0]);
        } else {
          succ = ObliviousNodeWrite(client->path, client->path_stash, stash_i,
              client->temp_node->block_contents + i,
              client->temp_node->block_keys[i],
              client->temp_node->block_paths[i],
              block_size,
              client->temp_block_content[0]);
        }
      }

      if (client->temp_node->block_keys[i]) client->stash_load += 1;

      if (client->temp_node->block_keys[i] && !succ) {
        printf_fmt("Error: Cannot fetch a path into the stash.\n");
      }
      assert(client->temp_node->block_keys[i] == 0 || succ);
    }
    addr = addr * 2 + 1 + tree_path % 2;
    tree_path /= 2;
  }
}

void ecall_download_path_to_enclave_client_aes(EncMemTreeNode *all_nodes,
                                               uint32_t height,
                                               uint32_t block_path,
                                               uint32_t block_size) {
  // assert(height == client->client_path_height);
  uint32_t addr = 0;
  uint32_t tree_path = (uint32_t) block_path;
  for (uint32_t h = 0; h < height; h++) {
    EncMemTreeNode *enc_node = all_nodes + addr;
    decryptData((uint8_t *)enc_node, sizeof(EncMemTreeNode),
                (uint8_t *)(client->path + h), sizeof(MemTreeNode));
    for (uint32_t i = 0; i < BLOCKS_PER_NODE; i++) {
      decryptData(
          (uint8_t *)(client->path[h].block_contents[i]),
          block_size + SGX_AESGCM_MAC_SIZE + SGX_AESGCM_IV_SIZE,
          (uint8_t *)(client->path_stash[h][i]),
          block_size);
    }
    addr = addr * 2 + 1 + tree_path % 2;
    tree_path /= 2;
  }
}

void ecall_download_path_to_enclave_client(MemTreeNode *all_nodes,
                                           uint32_t height,
                                           uint32_t block_path,
                                           uint32_t block_size) {
  // assert(height == client->client_path_height);
  uint32_t addr = 0;
  uint32_t tree_path = (uint32_t) block_path;
  for (uint32_t h = 0; h < height; h++) {
    memcpy(client->path + h, all_nodes + addr, sizeof(MemTreeNode));
    for (uint32_t i = 0; i < BLOCKS_PER_NODE; i++) {
      memcpy(client->path_stash[h][i],
             all_nodes[addr].block_contents[i],
             block_size);
    }
    addr = addr * 2 + 1 + tree_path % 2;
    tree_path /= 2;
  }
}

void ecall_evict_path_outside_enclave_path_oram(EncMemTreeNode *all_nodes,
                                                const uint32_t height,
                                                const BlockPath block_path,
                                                uint32_t block_size) {
  // assert(height == client->client_path_height);
  // printf_fmt("Start evict path\n");
  uint32_t addr[32] = {0};
  uint32_t tree_path = (uint32_t) block_path;
  addr[0] = 0;
  for (uint32_t h = 0; h < height; h++) {
    addr[h + 1] = addr[h] * 2 + 1 + (tree_path % 2);
    tree_path /= 2;
  }

  uint32_t leading_one = 1;
  tree_path = (uint32_t) block_path;
  for (int32_t h = height - 1; h >= 0; h--) {
    uint32_t path_mask = (uint32_t)((leading_one << h) - 1);
    // printf_fmt("h: %d path mask: %d tree_path: %d \n", h, path_mask, tree_path);
    memset(client->temp_node, 0, sizeof(MemTreeNode));
    /*
    for (uint32_t i = 0; i < BLOCKS_PER_NODE; i++) {
      printf_fmt("i = %d: %d \n", i, client->temp_node->presence_bits[i]);
      printf_fmt("%ld \n", (uint64_t)client->temp_block_content[i]);
    }
    */
    // bool succ = false;
    // int32_t succ_count = 0;
    for (uint32_t stash_i = 0; stash_i < CLIENT_MAX_PATH; stash_i++)
    for (uint32_t stash_j = 0; stash_j < BLOCKS_PER_NODE; stash_j++) {
      bool sign = (client->path[stash_i].block_paths[stash_j] & path_mask) ==
          (tree_path & path_mask)
          && (client->temp_node->num_blocks < BLOCKS_PER_NODE)
          && (client->path[stash_i].presence_bits[stash_j] > 0);
          // && (succ_count < BLOCKS_PER_NODE);
      // printf_fmt("%d %d\n", stash_i, stash_j);
      ObliviousNodeTransfer(client->temp_node, client->temp_block_content,
                            client->path[stash_i].block_keys[stash_j],
                            client->path[stash_i].block_paths[stash_j],
                            client->path[stash_i].block_contents[stash_j],
                            client->path_stash[stash_i][stash_j],
                            sign, block_size);
      if (sign) {
        client->path[stash_i].presence_bits[stash_j] = 0;
        client->path[stash_i].num_blocks -= 1;
        client->stash_load -= 1;
        /*
        printf_fmt("Write a block to height %d: %lu, %lu, %ld\n",
            h,
            client->path[stash_i].block_keys[stash_j],
            client->path[stash_i].block_paths[stash_j],
            (uint64_t)client->path[stash_i].block_contents[stash_j]);
        print_path_binary(client->path[stash_i].block_paths[stash_j]);
        print_path_binary(tree_path);
        print_path_binary(path_mask);
        */
      }
      // succ = succ || sign;
      // succ_count += 1;
    }

    for (uint32_t i = 0; i < BLOCKS_PER_NODE; i++) {
      // Restore the addr of encrypted data.
      client->temp_node->block_contents[i]
          = client->encrypted_data_addr_in_path[h][i];
    }

    EncMemTreeNode *enc_node = all_nodes + addr[h];
    // printf_fmt("Ok\n");
    encryptData((uint8_t *)(client->temp_node), sizeof(MemTreeNode),
                (uint8_t *)enc_node, sizeof(EncMemTreeNode));
    // printf_fmt("OK222\n");
    for (uint32_t i = 0; i < BLOCKS_PER_NODE; i++) {
      if (!client->temp_node->presence_bits[i]) {
        encryptData(
          (uint8_t *)(client->dummy_block),
          block_size,
          (uint8_t *)(client->temp_node->block_contents[i]),
          block_size + SGX_AESGCM_MAC_SIZE + SGX_AESGCM_IV_SIZE);
        continue;
      }
      // printf_fmt("i = %d: %d \n", i, client->temp_node->presence_bits[i]);
      // printf_fmt("%ld \n", (uint64_t)client->temp_block_content[i]);
      encryptData(
          (uint8_t *)(client->temp_block_content[i]),
          block_size,
          (uint8_t *)(client->temp_node->block_contents[i]),
          block_size + SGX_AESGCM_MAC_SIZE + SGX_AESGCM_IV_SIZE);
    }
    //printf_fmt("OK333\n");
  }
  //printf_fmt("Stash load: %d\n", client->stash_load);
}

/* Upload the path from client to the server (in-memory setting). */
void ecall_update_path_outside_enclave_aes(EncMemTreeNode *all_nodes,
                                           const uint32_t height,
                                           const BlockPath block_path,
                                           uint32_t block_size) {
  // assert(height == client->client_path_height);
  uint32_t addr = 0;
  uint32_t tree_path = (uint32_t) block_path;
  for (uint32_t h = 0; h < height; h++) {
    EncMemTreeNode *enc_node = all_nodes + addr;
    encryptData((uint8_t *)(client->path + h), sizeof(MemTreeNode),
                (uint8_t *)enc_node, sizeof(EncMemTreeNode));
    for (uint32_t i = 0; i < BLOCKS_PER_NODE; i++) {
      encryptData(
          (uint8_t *)(client->path_stash[h][i]),
          block_size,
          (uint8_t *)(client->path[h].block_contents[i]),
          block_size + SGX_AESGCM_MAC_SIZE + SGX_AESGCM_IV_SIZE);
    }
    addr = addr * 2 + 1 + tree_path % 2;
    tree_path /= 2;
  }
}

/* Upload the path from client to the server (in-memory setting). */
void ecall_update_path_outside_enclave(MemTreeNode *all_nodes,
                                       const uint32_t height,
                                       const BlockPath block_path,
                                       uint32_t block_size) {
  // assert(height == client->client_path_height);
  uint32_t addr = 0;
  uint32_t tree_path = (uint32_t) block_path;
  for (uint32_t h = 0; h < height; h++) {
    memcpy(all_nodes + addr, client->path + h, sizeof(MemTreeNode));
    for (uint32_t i = 0; i < BLOCKS_PER_NODE; i++) {
      memcpy(all_nodes[addr].block_contents[i],
             client->path_stash[h][i],
             block_size);
    }
    addr = addr * 2 + 1 + tree_path % 2;
    tree_path /= 2;
  }
}

/* Obliviously fetch the block with given key on client side.
 * If block_key does not exist, return a zero block.
 * If block_key is zero, return a zero block. */
void ecall_oblivious_fetch(const uint32_t height, const uint32_t block_size,
                           const BlockKey block_key,
                           uint8_t *fetched_block,
                           uint8_t **encrypted_content_ptr) {
  // assert(height == client->client_path_height);
  memset(fetched_block, 0, block_size);
  bool succ = false;
  for (uint32_t h = 0; h < CLIENT_MAX_PATH; h++) {
    succ = succ || ObliviousNodeRead(
        client->path, client->path_stash, h,
        block_key, block_size, fetched_block, encrypted_content_ptr);
  }
  if (succ) client->stash_load -= 1;
}

/* Obliviously put the block with key on root node on client side. */
void ecall_oblivious_put_to_stash_path_oram(EncMemTreeNode *all_nodes,
                                            const uint32_t height,
                                            const uint32_t block_size,
                                            const BlockPath block_path,
                                            const BlockKey block_key,
                                            uint8_t **encrypted_content_ptr,
                                            uint8_t *block_content,
                                            uint32_t *root_load) {
  bool succ = false;

  for (uint32_t h = 0; h < CLIENT_MAX_PATH; h++) {
    if (!succ && client->path[h].num_blocks < BLOCKS_PER_NODE) {
      succ = ObliviousNodeWrite(
          client->path, client->path_stash, h, encrypted_content_ptr,
          block_key, block_path, block_size, block_content);
      /*
      printf_fmt("Succ write at height %d: %lu, %lu, %ld\n",
          h,
          block_key,
          block_path,
          (uint64_t)(*encrypted_content_ptr));
      for (int ii = 0; ii < BLOCKS_PER_NODE; ii += 1) {
        printf_fmt("ii: %d, %lu, %lu, %ld\n",
          ii,
          client->path[h].block_keys[ii],
          client->path[h].block_paths[ii],
          (uint64_t)client->path[h].block_contents[ii]);
      }
      */
    } else {
      ObliviousNodeWrite(
          client->path, client->path_stash, h, encrypted_content_ptr,
          0, block_path, block_size, block_content);
    }
  }
  client->stash_load += 1;
  *root_load = client->stash_load;
  assert(succ);
  /*
  for (uint32_t stash_i = 0; stash_i < CLIENT_MAX_PATH; stash_i++) {
    printf_fmt("Number of blocks: %d\n", client->path[stash_i].num_blocks);
    for (uint32_t stash_j = 0; stash_j < BLOCKS_PER_NODE; stash_j++) {
      printf_fmt("block_key: %d, block_path: %d, block_content: %ld, presence: %d\n",
        client->path[stash_i].block_keys[stash_j],
        client->path[stash_i].block_paths[stash_j],
        (uint64_t)client->path[stash_i].block_contents[stash_j],
        client->path[stash_i].presence_bits[stash_j]);
    }
  }
  */
}

/* Obliviously put the block with key on root node on client side. */
void ecall_oblivious_put_to_root_aes(EncMemTreeNode *all_nodes,
                                     const uint32_t height,
                                     const uint32_t block_size,
                                     const BlockPath block_path,
                                     const BlockKey block_key,
                                     uint8_t *block_content,
                                     uint32_t *root_load) {
  decryptData((uint8_t *)all_nodes, sizeof(EncMemTreeNode),
              (uint8_t *)client->path, sizeof(MemTreeNode));
  for (uint32_t i = 0; i < BLOCKS_PER_NODE; i++) {
    decryptData(
        (uint8_t *)(client->path[0].block_contents[i]),
        block_size + SGX_AESGCM_MAC_SIZE + SGX_AESGCM_IV_SIZE,
        (uint8_t *)(client->path_stash[0][i]),
        block_size);
  }

  if (client->path[0].num_blocks >= BLOCKS_PER_NODE) {
    printf_fmt("Flush error: Cannot put into the root node.\n");
    assert(false);
  }

  assert(ObliviousNodeWrite(
      client->path, client->path_stash, 0, nullptr,
      block_key, block_path, block_size, block_content));

  encryptData((uint8_t *)client->path, sizeof(MemTreeNode),
              (uint8_t *)all_nodes, sizeof(EncMemTreeNode));
  for (uint32_t i = 0; i < BLOCKS_PER_NODE; i++) {
    encryptData(
        (uint8_t *)(client->path_stash[0][i]),
        block_size,
        (uint8_t *)(client->path[0].block_contents[i]),
        block_size + SGX_AESGCM_MAC_SIZE + SGX_AESGCM_IV_SIZE);
  }

  *root_load = client->path[0].num_blocks;
}

/* Obliviously put the block with key on root node on client side. */
void ecall_oblivious_put_to_root(MemTreeNode *all_nodes, const uint32_t height,
                                 const uint32_t block_size,
                                 const BlockPath block_path,
                                 const BlockKey block_key,
                                 uint8_t *block_content,
                                 uint32_t *root_load) {
  memcpy(client->path, all_nodes, sizeof(MemTreeNode));
  for (int i = 0; i < BLOCKS_PER_NODE; i++) {
    memcpy(client->path_stash[0][i],
           all_nodes[0].block_contents[i],
           block_size);
  }

  assert(client->path[0].num_blocks < BLOCKS_PER_NODE);
  assert(ObliviousNodeWrite(
      client->path, client->path_stash, 0, nullptr,
      block_key, block_path, block_size, block_content));

  memcpy(all_nodes, client->path, sizeof(MemTreeNode));
  for (uint32_t i = 0; i < BLOCKS_PER_NODE; i++) {
    memcpy(all_nodes[0].block_contents[i],
           client->path_stash[0][i],
           block_size);
  }

  *root_load = client->path[0].num_blocks;
}

/* PathORAM: Flush the path on client side. */
void ecall_client_simple_oram_flush(uint32_t height,
                                    uint32_t block_size,
                                    uint32_t rand_path) {
  // assert(height == client->client_path_height);
  // This special case is needed; otherwise the next for loop cannot exit.
  if (height < 2) return;
  
  for (uint32_t i = 0; i < height - 1; i++) {
    for (uint32_t j = 0; j < BLOCKS_PER_NODE; j++) {
      bool is_empty_block = (client->path[i].presence_bits[j] == 0);
      // TODO: A reasonable compromise? skip empty blocks.
      if (is_empty_block) continue;
      uint8_t *block = client->path_stash[i][j];
      const BlockKey block_key = client->path[i].block_keys[j];
      const BlockPath block_path = client->path[i].block_paths[j];

      bool has_flushed = false;
      bool failed_flushed = false;
      bool is_path_match, will_insert;
      for (uint32_t k = i; k < height - 1; k++) {
        // printf_fmt("%d, %d", rand_path, block_key);

        is_path_match = (rand_path & (1 << k)) == (block_path & (1 << k));
        failed_flushed = failed_flushed || (!is_path_match);
        will_insert = (!failed_flushed) && (!has_flushed) && is_path_match;

        // printf_fmt("Start node write.%d, %d, %d, %d \n", k, block_key, inserted, block_key & (-inserted));

        will_insert = will_insert && ObliviousNodeWrite(
            client->path, client->path_stash, k + 1, nullptr,
            block_key & (-will_insert), block_path, block_size, block);
        
        // printf_fmt("%d %d\n", k + 1, will_insert);

        // client->path[i].block_keys[j] &= -(will_insert == false);
        // client->path[i].block_paths[j] &= -(will_insert == false); 
        client->path[i].presence_bits[j] &= (-(will_insert == false));
        client->path[i].num_blocks -= will_insert;
        has_flushed = has_flushed || will_insert;
      }
    }
  }
}
