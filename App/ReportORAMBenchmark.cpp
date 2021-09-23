#include <cassert>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "App.h"
#include "Enclave_u.h"
#include "PathORAM.h"

void report_oram_benchmark(const char* oram_file_name,
                           int num_blocks,
                           int block_size) {
  /*
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;
  char *message = "Hello, this is Jinjin.";
  uint32_t encMessageLen = (SGX_AESGCM_MAC_SIZE + SGX_AESGCM_IV_SIZE + strlen(message));
  char *encMessage = (char *)malloc((encMessageLen + 1) * sizeof(char));
  std::cout << encMessageLen << std::endl;
  ret = ecall_encryptData(global_eid,
      (uint8_t *)message, strlen(message), (uint8_t *)encMessage, encMessageLen);
  encMessage[encMessageLen] = '\0';
  std::cout << "Message: " << message << " Encryption: " << encMessage 
            << "Status: " << std::hex << ret << std::endl;
  assert(ret == SGX_SUCCESS);

  size_t decMessageLen = strlen(message);
  char *decMessage = (char *)malloc((decMessageLen + 1) * sizeof(char));
  
  ecall_decryptData(global_eid,
      (uint8_t *)encMessage, encMessageLen, (uint8_t *)decMessage, decMessageLen);
  decMessage[decMessageLen] = '\0';
  std::cout << "Decryption: " << decMessage << std::endl;

  const uint32_t node_size = 1024;
  uint8_t *empty_node = (uint8_t *)calloc(1024, node_size);
  uint8_t *enc_node = (uint8_t *)calloc(1024, node_size + 28);
  std::cout << 1LL + sizeof(MemTreeNode) << " " << 1LL + sizeof(EncMemTreeNode) << std::endl;

  ret = ecall_encryptData(global_eid,
      empty_node, node_size,
      enc_node, node_size + 28);
  std::cout << "Encryption satus: " << std::hex << ret << std::endl;
  assert(ret == SGX_SUCCESS);

  return;
  */

  /*
  uint32_t sort_test_len = block_size;
  auto start_11 = std::chrono::high_resolution_clock::now();
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;
  ret = ecall_test_osort(global_eid, sort_test_len, 1);
  assert(ret == SGX_SUCCESS);
  auto end_11 = std::chrono::high_resolution_clock::now();
  std::cout << "Time cost in millisec: "
            << std::chrono::duration_cast<std::chrono::microseconds>(end_11 - start_11).count() * 1.0 / 1000.0
            << std::endl;
  auto start_22 = std::chrono::high_resolution_clock::now();
  ret = ecall_test_osort(global_eid, sort_test_len, 0);
  assert(ret == SGX_SUCCESS);
  auto end_22 = std::chrono::high_resolution_clock::now();
  std::cout << "Time cost in millisec: "
            << std::chrono::duration_cast<std::chrono::microseconds>(end_22 - start_22).count() * 1.0 / 1000.0
            << std::endl;
  return;  
  */

  std::cout << "Start ORAM Benchmark..." << std::endl;

  std::cout << "Init an ORAM " << oram_file_name << " "
            << num_blocks << "*"
            << block_size << "..." << std::endl;

  assert(num_blocks > 0 && block_size > 0);

  std::cout << "Test correctness..." << std::endl;

  uint8_t* data = (uint8_t *) malloc(block_size * sizeof(uint8_t));
  for (int i = 0; i < block_size; i++) data[i] = (i * i) % 13;
  // SimpleORAM *test_oram = new SimpleORAM(num_blocks, block_size);
  PathORAM *test_oram = new PathORAM(num_blocks, block_size);
  assert(test_oram->write(61, data, block_size, 0));
  for (int i = 0; i < block_size; i++) data[i] = (i * i) % 19;
  assert(test_oram->write(19, data, block_size, 0));
  for (int i = 0; i < block_size; i++) data[i] = (i * i) % 23;
  assert(test_oram->write(23, data, block_size, 0));

  memset(data, 0, block_size);

  assert(test_oram->read(19, data));

  bool succ = true;
  for (int i = 0; i < block_size; i++)
    if (data[i] != (i * i) % 19) {
#ifdef DEBUG_ORAM_ACCESS
      std::cout << "data[" << i << "]: "
                << (int)data[i] << ", " << (i * i) % 19 << std::endl;
#endif
      succ = false;
    }

  if (succ) {
    std::cout << "Succeed." << std::endl;
  } else {
    std::cout << "Failed." << std::endl;
    abort();
  }

  std::cout << "Write 1000 random blocks..." << std::endl;
  
  std::srand(std::time(nullptr));

  int block_ids[1000] = {0};
  for (int i = 0; i < 1000; i++) {
    block_ids[i] = std::rand() % num_blocks;
  }

  auto start = std::chrono::high_resolution_clock::now(); 
  for (int i = 0; i < 1000; i++) {
#ifdef DEBUG_ORAM_ACCESS
    std::cout << "Write block: " << block_ids[i] << std::endl;
#endif

    test_oram->write(block_ids[i], data, block_size, 0);
  }
  auto end = std::chrono::high_resolution_clock::now();

  std::cout << "Avg time cost in millisec: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() * 1.0 / 1000.0
            << std::endl;

  std::cout << "Read 1000 random blocks..." << std::endl;

  for (int i = 0; i < 1000; i++) {
    block_ids[i] = std::rand() % num_blocks;
  }

  start = std::chrono::high_resolution_clock::now(); 
  for (int i = 0; i < 1000; i++) {
    test_oram->read(61, data);  
  }
  end = std::chrono::high_resolution_clock::now();

  std::cout << "Avg time cost in millisec: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() * 1.0 / 1000.0
            << std::endl;

  free(data);
}
