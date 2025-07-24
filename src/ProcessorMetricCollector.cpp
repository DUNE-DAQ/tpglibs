/**
 * @file ProcessorMetricCollector.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/ProcessorMetricCollector.hpp"
#include "tpglibs/ProcessorMetricArray.hpp"
#include "tpglibs/AbstractProcessor.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <immintrin.h> 

namespace tpglibs {

template class ProcessorMetricCollector<__m256i>;

template<typename signal_t>
std::vector<AbstractProcessor<signal_t>*> ProcessorMetricCollector<signal_t>::_get_attached_processors() {
  return m_attached_processors;
}

template<typename signal_t>
std::vector<ProcessorMetricInformation> ProcessorMetricCollector<signal_t>::_get_processor_metric_table() {
  return m_processor_metric_table;
}

template<typename signal_t>
std::vector<std::vector<std::vector<int16_t>>> ProcessorMetricCollector<signal_t>::_get_processor_casted_data_table() {
  return m_processor_casted_data_table;
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
  m_attached_processors = std::vector<AbstractProcessor<signal_t>*>(n_processors * num_pipelines);

  m_processor_metric_table = std::vector<ProcessorMetricInformation>(n_processors * num_pipelines, ProcessorMetricInformation{0, {}});

  m_processor_metric_collection_table = {};

  m_channel_numbers = std::vector<dunedaq::trgdataformats::channel_t>(channel_plane_numbers.size(), 0);

  for (size_t i = 0; i < channel_plane_numbers.size(); i++) {
    m_channel_numbers[i] = channel_plane_numbers[i].first;
    m_metrics[m_channel_numbers[i]] = std::vector<std::pair<std::string, int16_t>>();
  }
}

template<typename signal_t>
void ProcessorMetricCollector<signal_t>::collect_metrics_from_attached_processors() {
  size_t proc_id = 0;
  for (auto processor_ptr : m_attached_processors) {
    if (!processor_ptr) continue;
    ProcessorMetricArray items = processor_ptr->read_from_metric_store_buffer();
    for (size_t i = 0; i < items.m_size; i++) {
      m_processor_metric_collection_table[proc_id][i] = items.m_data[i];
    }
    proc_id++;
  }
}

template<typename signal_t>
void ProcessorMetricCollector<signal_t>::cast_metrics_from_raw_type() {
  for (size_t i = 0; i < m_processor_metric_collection_table.size(); i++) {
    auto raw = m_processor_metric_collection_table[i];
    for (size_t j = 0; j < raw.size(); j++) {
      int16_t out[16];
      _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out), raw[j]);
      for (size_t k = 0; k < 16; k++) {
        m_processor_casted_data_table[i][j][k] = out[k];
      }
    }
  }
}

template<typename signal_t>
void ProcessorMetricCollector<signal_t>::signal_collect() {
  // TODO: signal that a collection cycle should occur
  m_signal_collect.store(true, std::memory_order_release);
  // this->collect_metrics_from_attached_processors();
  // this->cast_metrics_from_raw_type();
}

template<typename signal_t>
void ProcessorMetricCollector<signal_t>::run() {
  // TODO: main loop for metric collection thread
  m_collector_thread = std::thread([this]() {
    while (!this->m_stop_flag.load(std::memory_order_acquire)) {
      if (this->m_signal_collect.load(std::memory_order_acquire)) {
        // If this is signaled to collect metrics
        this->collect_metrics_from_attached_processors();

        this->m_signal_collect.store(false, std::memory_order_release);
        // After collection, we reset the collect flag to false.
        // Note that when signal_collect() is called at a far higher rate then possible, ultimately collection
        // happens at the highest possible rate, not necessarily the set rate

        // perform any post processings on the collected metrics as needed
        this->cast_metrics_from_raw_type();
        this->convert_into_channel_metric_value();
      }
    }
  });
}

template<typename signal_t>
std::vector<std::vector<signal_t>> 
ProcessorMetricCollector<signal_t>::_get_retrieved_processor_metrics() const {
  // TODO: return collected metrics
  return m_processor_metric_collection_table;
}

template<typename signal_t>
void ProcessorMetricCollector<signal_t>::stop() {
  // TODO: stop the collection thread
  m_stop_flag.store(true, std::memory_order_release);
  if (m_collector_thread.joinable()) {
    m_collector_thread.join();
  }
}

template<typename signal_t>
void ProcessorMetricCollector<signal_t>::convert_into_channel_metric_value() {
  std::vector<int16_t> mcount(m_channel_numbers.size(), 0);

  for (size_t pid = 0; pid < m_processor_casted_data_table.size(); pid++) {
    for (size_t mid = 0; mid < m_processor_casted_data_table[pid].size(); mid++) {
      for (size_t cid = 0; cid < m_processor_casted_data_table[pid][mid].size(); cid++) {
        auto mnames = m_processor_metric_table[pid].m_names_of_metrics;
        m_metrics[m_channel_numbers[cid]][mcount[cid]].second = m_processor_casted_data_table[pid][mid][cid];
        m_metrics[m_channel_numbers[cid]][mcount[cid]].first = m_processor_metric_table[pid].m_names_of_metrics[mid];
        mcount[cid]++;
      }
    }
  }
}

template<typename signal_t>
std::unordered_map<dunedaq::trgdataformats::channel_t, std::vector<std::pair<std::string, int16_t>>> ProcessorMetricCollector<signal_t>::get_metrics() {
  return m_metrics;
}

} // namespace tpglibs