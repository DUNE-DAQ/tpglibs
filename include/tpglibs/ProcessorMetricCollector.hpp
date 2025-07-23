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
#include <unordered_map>
#include <vector>
#include <thread>
#include <atomic>
#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>

#include "trgdataformats/Types.hpp"

namespace tpglibs {

template <typename T> class AbstractProcessor;


struct ProcessorMetricInformation {
  int16_t m_pipeline_id;
  std::vector<std::string> m_names_of_metrics;
};

// Idealy this exists at config level. Hardcoded here for now
inline std::unordered_map<std::string, std::vector<std::string>> processor_name_to_metrics_map = {
  {"AVXFrugalPedestalSubtractProcessor", {"m_pedestal", "m_accum"}}
};

template <typename T>
class ProcessorMetricCollector {
public:

  using signal_t = T;

  void attach_processor(AbstractProcessor<signal_t>& processor, const std::string& processor_type_name,
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

  void configure(const std::vector<std::pair<std::string, nlohmann::json>> configs,
                 const std::vector<std::pair<dunedaq::trgdataformats::channel_t, int16_t>> channel_plane_numbers,
                 uint8_t num_pipelines);

  std::shared_ptr<AbstractProcessor<signal_t>*[]> _get_attached_processors();
  std::map<int16_t, ProcessorMetricInformation> _get_processor_metric_table();
  void collect_metrics_from_attached_processors();
  void cast_metrics_from_raw_type();
  void signal_collect();
  void run();  // main loop on its dedicated thread
  std::map<int16_t, std::vector<ProcessorMetricInformation>> get_retrieved_processor_metrics() const;
  void stop();

private:
  std::shared_ptr<AbstractProcessor<signal_t>*[]> m_attached_processors;
  std::map<int16_t, ProcessorMetricInformation> m_processor_metric_table;
  std::vector<std::vector<signal_t>> m_processor_metric_collection_table;
  
  std::atomic<bool> m_signal_collect{false};
  std::thread m_collector_thread;
  std::atomic<bool> m_stop_flag{false};
  size_t m_attach_counter;
};

} // namespace tpglibs
 
 #endif // TPGLIBS_PROCESSORMETRICCOLLECTOR_HPP_