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

namespace tpglibs {


 // Idealy this exists at config level. Hardcoded here for now
std::unordered_map<std::string, std::vector<std::string>> processor_name_to_metrics_map = {
  {"AVXFrugalPedestalSubtractProcessor", {"m_pedestal", "m_accum"}}
};

void ProcessorMetricCollector::attach_processor(AbstractProcessor<__m256i>& processor, std::string processor_type_name,
                                                size_t pipeline_id) {
  // Attach a processor to be observed (collected) by this
  m_attached_processors[m_attach_counter] = &processor;

  // Look up and fill in information about this processor: its pipeline belonging and what is collected
  auto metrics = processor_name_to_metrics_map[processor_type_name];
  auto metric_info = ProcessorMetricInformation();
  metric_info.m_names_of_metrics = metrics;
  metric_info.m_pipeline_id = pipeline_id;
  // store this info
  m_processor_metric_table[m_attach_counter] = metric_info;

  m_attach_counter++;
}

std::shared_ptr<AbstractProcessor<__m256i>*[]> ProcessorMetricCollector::_get_attached_processors() {
  return m_attached_processors;
}

std::map<int16_t, ProcessorMetricInformation> ProcessorMetricCollector::_get_processor_metric_table() {
  return m_processor_metric_table;
}

void ProcessorMetricCollector::configure(const std::vector<std::pair<std::string, nlohmann::json>> configs,
                                         const std::vector<std::pair<dunedaq::trgdataformats::channel_t, int16_t>> channel_plane_numbers,
                                         uint8_t num_pipelines)
{
  m_signal_collect = false;
  m_attach_counter = 0;

  // create a fixed size array for reference to all the processors
  // configs are pairs of (processor name, specific configs)
  size_t n_processors = configs.size();
  m_attached_processors = std::make_unique<AbstractProcessor<__m256i>*[]>(n_processors * num_pipelines);

  // Constructor stub: initialize collector with configs and channel_plane_numbers

  m_processor_metric_table = {};

  // Initialize empty table, containing the information regarding metric of each processor

  for (size_t i = 0; i < n_processors * num_pipelines; i++) {
    m_processor_metric_table[i] = ProcessorMetricInformation{0, {}};
  }

}

void ProcessorMetricCollector::collect_metrics_from_attached_processors() {
  // TODO: collect metrics from attached processors
}

void ProcessorMetricCollector::cast_metrics_from_raw_type() {
  // TODO: cast raw metrics to useful format
}

void ProcessorMetricCollector::signal_collect() {
  // TODO: signal that a collection cycle should occur
  m_signal_collect.store(true, std::memory_order_release);
}

void ProcessorMetricCollector::run() {
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

std::map<int16_t, std::vector<ProcessorMetricInformation>>
ProcessorMetricCollector::get_retrieved_processor_metrics() const {
  // TODO: return collected metrics
  return {};
}

void ProcessorMetricCollector::stop() {
  // TODO: stop the collection thread
  m_stop_flag.store(true, std::memory_order_release);
  if (m_collector_thread.joinable()) {
    m_collector_thread.join();
  }
}

} // namespace tpglibs