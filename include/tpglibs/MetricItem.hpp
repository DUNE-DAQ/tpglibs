// Definition of a single metric item
#pragma once
#include <cstdint>
#include <memory>
namespace tpglibs {

template<class T> // type of signal
struct metric_item {
  int16_t processor_id;
  int16_t pipeline_id;
  int16_t metric_id;
  std::unique_ptr<T> valueptr;
};

}//tpglibs
