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

#define MAX_PATH FILENAME_MAX

#include "sgx_urts.h"
#include "App.h"
#include "Enclave_u.h"

#include <sys/time.h>

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
  sgx_status_t err;
  const char *msg;
  const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {
        SGX_ERROR_UNEXPECTED,
        "Unexpected error occurred.", NULL
    },
    {
        SGX_ERROR_INVALID_PARAMETER,
        "Invalid parameter.", NULL
    },
    {
        SGX_ERROR_OUT_OF_MEMORY,
        "Out of memory.", NULL
    },
    {
        SGX_ERROR_ENCLAVE_LOST,
        "Power transition occurred.",
        "Please refer to the sample \"PowerTransition\" for details."
    },
    {
        SGX_ERROR_INVALID_ENCLAVE,
        "Invalid enclave image.", NULL
    },
    {
        SGX_ERROR_INVALID_ENCLAVE_ID,
        "Invalid enclave identification.", NULL
    },
    {
        SGX_ERROR_INVALID_SIGNATURE,
        "Invalid enclave signature.", NULL
    },
    {
        SGX_ERROR_OUT_OF_EPC,
        "Out of EPC memory.", NULL
    },
    {
        SGX_ERROR_NO_DEVICE,
        "Invalid SGX device.",
        "Please make sure SGX module is enabled in the BIOS, and install SGX "
        "driver afterwards."
    },
    {
        SGX_ERROR_MEMORY_MAP_CONFLICT,
        "Memory map conflicted.", NULL
    },
    {
        SGX_ERROR_INVALID_METADATA,
        "Invalid enclave metadata.", NULL
    },
    {
        SGX_ERROR_DEVICE_BUSY,
        "SGX device was busy.", NULL
    },
    {
        SGX_ERROR_INVALID_VERSION,
        "Enclave version was invalid.", NULL
    },
    {
        SGX_ERROR_INVALID_ATTRIBUTE,
        "Enclave was not authorized.", NULL
    },
    {
        SGX_ERROR_ENCLAVE_FILE_ACCESS,
        "Can't open enclave file.", NULL
    },
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret) {
  size_t idx = 0;
  size_t ttl = sizeof sgx_errlist / sizeof sgx_errlist[0];

  for (idx = 0; idx < ttl; idx++) {
    if (ret == sgx_errlist[idx].err) {
      if (NULL != sgx_errlist[idx].sug)
        printf("Info: %s\n", sgx_errlist[idx].sug);
      printf("Error: %s\n", sgx_errlist[idx].msg);
      break;
    }
  }

  if (idx == ttl)
    printf(
        "Error code is 0x%X. Please refer to the \"Intel SGX SDK Developer "
        "Reference\" for more details.\n",
        ret);
}

/* Initialize the enclave:
 *   Call sgx_create_enclave to initialize an enclave instance
 */
int initialize_enclave(void) {
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;

  /* Call sgx_create_enclave to initialize an enclave instance */
  /* Debug Support: set 2nd parameter to 1 */
  ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL,
                           &global_eid, NULL);
  if (ret != SGX_SUCCESS) {
    print_error_message(ret);
    return -1;
  }

  return 0;
}

/* OCall functions */
void ocall_print_string(const char *str)
{
    /* Proxy/Bridge will check the length and null-terminate 
     * the input string to prevent buffer overflow. */
    printf("%s", str);
}

void ocall_sleep(uint64_t time_ms)
{
  usleep(time_ms * 1000);
}

void ocall_print_time(uint64_t* time_us)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  *time_us = (tv.tv_sec) * 1000 * 1000 + (tv.tv_usec);
}

/* Application entry */
int SGX_CDECL main(int argc, char *argv[]) {
  /* Initialize the enclave; Error messages are inside the init function */
  if (initialize_enclave() < 0) {
    fprintf(stderr, "Enter a character before exit ...\n");
    getchar();
    return -1;
  }

  if (argc == 1)
  {
    fprintf(stderr, "./app [task-name] [options]\n\n");
  }
  else if (!strcmp(argv[1], "oram_benchmark"))
  {
    if (argc == 5) {
      report_oram_benchmark(argv[2], atoi(argv[3]), atoi(argv[4]));
      /* Destroy the enclave */
      sgx_destroy_enclave(global_eid);
    } else {
      fprintf(stderr, "./app oram_benchmark [oram-file-name] [num-block] [block-size] \n\n");
    }
  }
  else if (!strcmp(argv[1], "build_basic_index"))
  {
    if (argc == 4) {
      // generate_basic_encrypted_index(argv[2], argv[3]);
      /* Destroy the enclave */
      sgx_destroy_enclave(global_eid);
    } else {
      fprintf(stderr, "./app build_basic_index [posting-file] [output-index]\n\n");
    }
  }
  else if (!strcmp(argv[1], "query_basic_index"))
  {
    if (argc == 7) {
      if (strcmp(argv[5], "clueweb") * strcmp(argv[5], "trec45") != 0) {
        fprintf(stderr, "Invalid dataset name.\n\n");
        return 0;
      } else if (strcmp(argv[6], "wave")
               * strcmp(argv[6], "wand")
               * strcmp(argv[6], "bmw") != 0) {
        fprintf(stderr, "Invalid merge metghod.\n\n");
        return 0;
      }

      // query_basic_encrypted_index(argv[2], argv[3], argv[4], argv[5], argv[6]);
      /* Destroy the enclave */
      sgx_destroy_enclave(global_eid);
    } else {
      fprintf(stderr,
          "./app query_basic_index"
          " [encrypted_index]"
          " [posting_info_file]"
          " [query_file]"
          " [dataset]"
          " [merge_method] \n\n");
    }
  }
  else if (!strcmp(argv[1], "build_oram_index"))
  {
    if (argc == 4) {
      // generate_oram_index(argv[2], argv[3]);
      /* Destroy the enclave */
      sgx_destroy_enclave(global_eid);
    } else {
      fprintf(stderr, "./app build_oram_index [posting-file] [output-index]\n\n");
    }
  }
  else if (!strcmp(argv[1], "query_oram_index"))
  {
    if (argc == 7) {
      if (strcmp(argv[5], "clueweb") * strcmp(argv[5], "trec45") != 0) {
        fprintf(stderr, "Invalid dataset name.\n\n");
        return 0;
      } else if (strcmp(argv[6], "wave")
               * strcmp(argv[6], "wand")
               * strcmp(argv[6], "bmw") != 0) {
        fprintf(stderr, "Invalid merge metghod.\n\n");
        return 0;
      }

      // query_oram_encrypted_index(argv[2], argv[3], argv[4], argv[5], argv[6]);
      /* Destroy the enclave */
      sgx_destroy_enclave(global_eid);
    } else {
      fprintf(stderr,
          "./app query_oram_index"
          " [encrypted_index]"
          " [posting_info_file]"
          " [query_file]"
          " [dataset]"
          " [merge_method] \n\n");
    }
  }
  else if (!strcmp(argv[1], "build_two_phase_index"))
  {
    if (argc == 4) {
      // generate_two_phase_index(argv[2], argv[3]);
      /* Destroy the enclave */
      sgx_destroy_enclave(global_eid);
    } else {
      fprintf(stderr, "./app build_two_phase_index [posting-file] [output-index]\n\n");
    }
  }
  else if (!strcmp(argv[1], "query_two_phase_index"))
  {
    if (argc == 7) {
      if (strcmp(argv[5], "clueweb") * strcmp(argv[5], "trec45") != 0) {
        fprintf(stderr, "Invalid dataset name.\n\n");
        return 0;
      } else if (strcmp(argv[6], "wave")
                 * strcmp(argv[6], "wand")
                 * strcmp(argv[6], "bmw")
                 * strcmp(argv[6], "vbmw")
                 * strcmp(argv[6], "tp") != 0) {
        fprintf(stderr, "Invalid merge metghod.\n\n");
        return 0;
      }

      // query_two_phase_index(argv[2], argv[3], argv[4], argv[5], argv[6]);
      /* Destroy the enclave */
      sgx_destroy_enclave(global_eid);
    } else {
      fprintf(stderr,
          "./app query_two_phase_index"
          " [encrypted_index]"
          " [posting_info_file]"
          " [query_file]"
          " [dataset]"
          " [merge_method] \n\n");
    }
  }
  else if (!strcmp(argv[1], "build_two_phase_index_variable_size"))
  {
    if (argc == 5) {
      // generate_two_phase_index_variable_size(argv[2], argv[3], argv[4]);
      /* Destroy the enclave */
      sgx_destroy_enclave(global_eid);
    } else {
      fprintf(stderr, "./app build_two_phase_index_variable_size [posting-file] [block-file] [output-index]\n\n");
    }
  }
  else if (!strcmp(argv[1], "build_two_phase_index_variable_size_v2"))
  {
    if (argc == 5) {
      // generate_two_phase_index_variable_size_v2(argv[2], argv[3], argv[4]);
      /* Destroy the enclave */
      sgx_destroy_enclave(global_eid);
    } else {
      fprintf(stderr, "./app build_two_phase_index_variable_size_v2 [posting-file] [block-file] [output-index]\n\n");
    }
  }
  else
  {
    fprintf(stderr, "Invalid task name.\n\n");
  }

  // printf("\nEnter a character before exit ...\n");
  // getchar();
  return 0;
}
