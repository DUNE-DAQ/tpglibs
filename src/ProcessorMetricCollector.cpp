/**
 * @file ProcessorMetricCollector.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/ProcessorMetricCollector.hpp"

namespace tpglibs {

ProcessorMetricCollector::ProcessorMetricCollector(const std::vector<std::string>& configs,
                                                   const std::vector<int16_t>& channel_plane_numbers,
                                                   uint8_t num_pipelines)
  : m_processor_metric_information_table(),
    m_processor_metric_collection_table(),
    m_signal_collect(false)
{
  // Constructor stub: initialize collector with configs and channel_plane_numbers
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
  std::thread reader_thread([this]() {
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
  reader_thread.detach();
}

std::map<int16_t, std::vector<ProcessorMetricInformation>>
ProcessorMetricCollector::get_retrieved_processor_metrics() const {
  // TODO: return collected metrics
  return {};
}

void ProcessorMetricCollector::stop() {
  // TODO: stop the collection thread
  m_stop_flag.store(true, std::memory_order_release);
}

} // namespace tpglibs