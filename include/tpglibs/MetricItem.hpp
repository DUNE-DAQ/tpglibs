// Definition of a single metric item
#pragma once
#include <cstdint>
#include <memory>
#include <tuple>
#include <functional>

#include "trgdataformats/Types.hpp"

namespace tpglibs {

template<class T> // type of signal
struct MetricItem {
  int16_t processor_id;
  int16_t pipeline_id;
  int16_t metric_id;
  std::shared_ptr<T> valueptr;
};

struct MetricKey {
int16_t processor_id;
int16_t pipeline_id;
int16_t metric_id;

bool operator==(MetricKey const& o) const noexcept {
   return std::tie(processor_id, pipeline_id, metric_id) == std::tie(o.processor_id, o.pipeline_id, o.metric_id);
  }
};

template<class T> 
struct ChannelAwareSignalPointer {

  dunedaq::trgdataformats::channel_t channel_number;
  int16_t index; //For example, 0-16 index for int16 in an __m256i
  std::shared_ptr<T> valueptr;

};

}//tpglibs

namespace std {
  template <>
  struct hash<tpglibs::MetricKey> {
    size_t operator()(tpglibs::MetricKey const& k) const noexcept {
      return (static_cast<size_t>(k.processor_id) << 32) ^
	     (static_cast<size_t>(k.pipeline_id) << 16) ^
	     (static_cast<size_t>(k.metric_id));
    }
  };
}
