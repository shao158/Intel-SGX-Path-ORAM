#ifndef _CONSTANT_H
#define _CONSTANT_H

#define STDOUT_AND_ABORT(s) do { \
    std::cout << s << std::endl; abort(); \
  } while (0) 

#define TP_ORAM_ENABLE

// In both index building and query processing stage.
const int kPostingORAMSize = 8192; 
const int kMetadataORAMSize = 1024;
const double kNumTotalDocuments = 33836981; // 528155 : 33836981

// Only in index building stage.
const int kIntervalErrorLimit = 0; // log(1 + TF) * doc_id_distance
const int kSparsityLimit = 0;
const int kMinCountPosting = 100; // Clueweb is 100, Trec45 is 1.

// Only in query processing stage.
const int kTopK = 10;
const int kMinORAMBlockCount = 100;
const double kApproxIntervalPercentail = 0.05;

#endif
