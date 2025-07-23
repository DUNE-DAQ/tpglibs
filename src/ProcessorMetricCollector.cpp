/**
 * @file ProcessorMetricCollector.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/ProcessorMetricCollector.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <immintrin.h> 

namespace tpglibs {

template class ProcessorMetricCollector<__m256i>;

template<typename signal_t>
std::shared_ptr<AbstractProcessor<signal_t>*[]> ProcessorMetricCollector<signal_t>::_get_attached_processors() {
  return m_attached_processors;
}

template<typename signal_t>
std::map<int16_t, ProcessorMetricInformation> ProcessorMetricCollector<signal_t>::_get_processor_metric_table() {
  return m_processor_metric_table;
}

template<typename signal_t>
void ProcessorMetricCollector<signal_t>::configure(const std::vector<std::pair<std::string, nlohmann::json>> configs,
                                                   const std::vector<std::pair<dunedaq::trgdataformats::channel_t, int16_t>> channel_plane_numbers,
                                                   uint8_t num_pipelines)
{
  m_signal_collect = false;
  m_attach_counter = 0;

  // create a fixed size array for reference to all the processors
  // configs are pairs of (processor name, specific configs)
  size_t n_processors = configs.size();
  m_attached_processors = std::make_unique<AbstractProcessor<signal_t>*[]>(n_processors * num_pipelines);

  // Constructor stub: initialize collector with configs and channel_plane_numbers

  m_processor_metric_table = {};

  // Initialize empty table, containing the information regarding metric of each processor

  for (size_t i = 0; i < n_processors * num_pipelines; i++) {
    m_processor_metric_table[i] = ProcessorMetricInformation{0, {}};
  }

  m_processor_metric_collection_table = {};

}

template<typename signal_t>
void ProcessorMetricCollector<signal_t>::collect_metrics_from_attached_processors() {
  // TODO: collect metrics from attached processors
}

template<typename signal_t>
void ProcessorMetricCollector<signal_t>::cast_metrics_from_raw_type() {
  // TODO: cast raw metrics to useful format
}

template<typename signal_t>
void ProcessorMetricCollector<signal_t>::signal_collect() {
  // TODO: signal that a collection cycle should occur
  m_signal_collect.store(true, std::memory_order_release);
}

template<typename signal_t>
void ProcessorMetricCollector<signal_t>::run() {
  // TODO: main loop for metric collection thread
  m_collector_thread = std::thread([this]() {
    while (!this->m_stop_flag.load(std::memory_order_acquire)) {
      if (this->m_signal_collect.load(std::memory_order_acquire)) {
        // If this is signaled to collect metrics
        this->collect_metrics_from_attached_processors();

        // After collection, we reset the collect flag to false.
        // Note that when signal_collect() is called at a far higher rate then possible, ultimately collection
        // happens at the highest possible rate, not necessarily the set rate
        this->m_signal_collect.store(false, std::memory_order_release);
      }
    }
  });
}

template<typename signal_t>
std::map<int16_t, std::vector<ProcessorMetricInformation>>
ProcessorMetricCollector<signal_t>::get_retrieved_processor_metrics() const {
  // TODO: return collected metrics
  return {};
}

template<typename signal_t>
void ProcessorMetricCollector<signal_t>::stop() {
  // TODO: stop the collection thread
  m_stop_flag.store(true, std::memory_order_release);
  if (m_collector_thread.joinable()) {
    m_collector_thread.join();
  }
}

} // namespace tpglibs