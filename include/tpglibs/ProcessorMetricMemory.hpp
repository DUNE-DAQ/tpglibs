#ifndef TPGLIBS_PROCESSORMETRICMEMORY_HPP_
#define TPGLIBS_PROCESSORMETRICMEMORY_HPP_

#include <cstdint>
#include <atomic>

namespace tpglibs {

  struct ProcessorMetricMemory {
      std::atomic<uint64_t> field0{0};
      std::atomic<uint64_t> field1{0};
      std::atomic<uint64_t> field2{0};
      std::atomic<uint64_t> field3{0};
  };

} // tpglibs

#endif // TPGLIBS_PROCESSORMETRICMEMORY_HPP_