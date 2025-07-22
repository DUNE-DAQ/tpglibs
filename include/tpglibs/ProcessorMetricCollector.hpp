/**
 * @file ProcessorMetricCollector.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
 
 #ifndef TPGLIBS_PROCESSORMETRICCOLLECTOR_HPP_
 #define TPGLIBS_PROCESSORMETRICCOLLECTOR_HPP_
 

#include <map>
#include <vector>
#include <thread>
#include <atomic>
#include <cstdint>
#include <immintrin.h>
#include <string>
#include <nlohmann/json.hpp>

#include <tpglibs/AbstractProcessor.hpp>
#include "trgdataformats/Types.hpp"

namespace tpglibs {

struct ProcessorMetricInformation {
  int16_t m_pipeline_id;
  std::vector<std::string> m_names_of_metrics;
};

class ProcessorMetricCollector {
public:
  void attach_processor(AbstractProcessor<__m256i>& processor, std::string processor_type_name, size_t pipeline_id);
  void configure(const std::vector<std::pair<std::string, nlohmann::json>> configs,
                 const std::vector<std::pair<dunedaq::trgdataformats::channel_t, int16_t>> channel_plane_numbers,
                 uint8_t num_pipelines);

  std::shared_ptr<AbstractProcessor<__m256i>*[]> _get_attached_processors();
  std::map<int16_t, ProcessorMetricInformation> _get_processor_metric_table();
  void collect_metrics_from_attached_processors();
  void cast_metrics_from_raw_type();
  void signal_collect();
  void run();  // main loop on its dedicated thread
  std::map<int16_t, std::vector<ProcessorMetricInformation>> get_retrieved_processor_metrics() const;
  void stop();

private:
  std::shared_ptr<AbstractProcessor<__m256i>*[]> m_attached_processors;
  std::map<int16_t, ProcessorMetricInformation> m_processor_metric_table;

  std::vector<ProcessorMetricInformation> m_processor_metric_information_table;
  std::vector<__m256i> m_processor_metric_collection_table;
  std::atomic<bool> m_signal_collect{false};
  std::thread m_collector_thread;
  std::atomic<bool> m_stop_flag{false};
  size_t m_attach_counter;
};

} // namespace tpglibs
 
 #endif // TPGLIBS_PROCESSORMETRICCOLLECTOR_HPP_
 