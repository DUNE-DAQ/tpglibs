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
struct IndexAwareSignalPointer {

  int16_t index; //For example, 0-16 index for int16 in an __m256i
  std::shared_ptr<T> valueptr;

};

struct MetricBufferKey {
  
  int16_t processor_id;
  int16_t pipeline_id;
  int16_t metric_id;
  dunedaq::trgdataformats::channel_t channel_number;
  
  bool operator==(MetricBufferKey const& o) const noexcept {
    return std::tie(processor_id, pipeline_id, metric_id, channel_number) == std::tie(o.processor_id, o.pipeline_id, o.metric_id, o.channel_number);
  }
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

  template <>
  struct hash<tpglibs::MetricBufferKey> {
    size_t operator()(tpglibs::MetricBufferKey const& k) const noexcept {
      size_t h1 = std::hash<int16_t>{}(k.processor_id);
      size_t h2 = std::hash<int16_t>{}(k.pipeline_id);
      size_t h3 = std::hash<int16_t>{}(k.metric_id);
      size_t h4 = std::hash<int16_t>{}(k.channel_number);

      size_t seed = h1;
      // pure magic
      seed ^= h2 + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
      seed ^= h3 + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
      seed ^= h4 + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
      return seed;
    }
  };
}
