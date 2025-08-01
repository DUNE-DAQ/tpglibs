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
#include <mutex>
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

template <typename T>
class ProcessorMetricCollector {
public:

  using signal_t = T;

  void attach_processor(AbstractProcessor<signal_t>& processor, const std::string& processor_type_name,
                        size_t pipeline_id) {
    // Attach a processor to be observed (collected) by this
    m_attached_processors[m_attach_counter] = &processor;

    // Look up and fill in information about this processor: its pipeline belonging and what is collected
    auto metrics = processor.get_metric_items();
    m_processor_metric_table[m_attach_counter].m_names_of_metrics = metrics;
    m_processor_metric_table[m_attach_counter].m_pipeline_id = pipeline_id;

    m_processor_casted_data_table.emplace_back(
        metrics.size(),
        std::vector<int16_t>(16)
    );

    // Then instantiate the space for storing collected metrics
    // Preallocated to preconfigured number of metrics
    m_processor_metric_collection_table.push_back(std::vector<signal_t>(metrics.size()));

    for (size_t i = 0; i < m_metrics.size(); i++) {
      for (size_t j = 0; j < metrics.size(); j++) {
        // Record metric name and initial value 0 for this channel
        m_metrics[m_channel_numbers[i]].emplace_back(metrics[j], int16_t{0});
      }
    }

    m_attach_counter++;
  }

  void configure(const std::vector<std::pair<std::string, nlohmann::json>> configs,
                 const std::vector<std::pair<dunedaq::trgdataformats::channel_t, int16_t>> channel_plane_numbers,
                 uint8_t num_pipelines);

  // These are method just for debug and tests
  std::vector<AbstractProcessor<signal_t>*> _get_attached_processors();
  std::vector<ProcessorMetricInformation> _get_processor_metric_table();
  std::vector<std::vector<std::vector<int16_t>>> _get_processor_casted_data_table();

  void signal_collect();
  void run();  // main loop on its dedicated thread
  std::vector<std::vector<signal_t>> _get_retrieved_processor_metrics() const;
  std::unordered_map<dunedaq::trgdataformats::channel_t, std::vector<std::pair<std::string, int16_t>>> get_metrics();
  void stop();

  ProcessorMetricCollector() 
    : m_signal_collect(false)
    , m_stop_flag(false)
    , m_attach_counter(0) {
  }

  void lock_metric_modify();
  void unlock_metric_modify();

private:
  void cast_metrics_from_raw_type();
  void collect_metrics_from_attached_processors();
  void convert_into_channel_metric_value();

  std::vector<AbstractProcessor<signal_t>*> m_attached_processors;
  std::vector<ProcessorMetricInformation> m_processor_metric_table;
  std::vector<std::vector<std::vector<int16_t>>> m_processor_casted_data_table;
  std::vector<std::vector<signal_t>> m_processor_metric_collection_table;

  std::vector<dunedaq::trgdataformats::channel_t> m_channel_numbers;

  std::unordered_map<dunedaq::trgdataformats::channel_t, std::vector<std::pair<std::string, int16_t>>> m_metrics;
  std::mutex m_mutex;
  
  std::atomic<bool> m_signal_collect{false};
  std::thread m_collector_thread;
  std::atomic<bool> m_stop_flag{false};
  size_t m_attach_counter = 0;
};

} // namespace tpglibs
 
 #endif // TPGLIBS_PROCESSORMETRICCOLLECTOR_HPP_