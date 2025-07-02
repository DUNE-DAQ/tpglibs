/**
 * @file TPGPipeline.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_TPGPIPELINE_HPP_
#define TPGLIBS_TPGPIPELINE_HPP_

#include "tpglibs/AbstractFactory.hpp"
#include "tpglibs/MetricItem.hpp"
#include "tpglibs/AbstractProcessor.hpp"

#include "trgdataformats/TriggerPrimitive.hpp"
#include "trgdataformats/Types.hpp"

#include <nlohmann/json.hpp>
#include <vector>

namespace tpglibs {

/**
 * @class TPGPipeline
 *
 * @brief Abstract class for the TPG pipeline.
 */
template <typename T, typename U>
class TPGPipeline {
  public:
    /** @brief  Processor type to use. Generally AVX. */
    using processor_t = T;
    /** @brief Signal type to use. Generally __m256i. */
    using signal_t = U;

    virtual ~TPGPipeline() = default;

    /**
     * @brief Configure the pieces to the pipeline.
     *
     * @param configs Vector of processors and configurations to be used.
     * @param channel_plane_numbers Vector of channel numbers and their plane numbers.
     */
    virtual void configure(const std::vector<std::pair<std::string, nlohmann::json>> configs, const std::vector<std::pair<dunedaq::trgdataformats::channel_t, int16_t>> channel_plane_numbers) {
      std::shared_ptr<processor_t> prev_processor = nullptr;

      for (int i = 0; i < 16; i++) {
        m_channels[i] = channel_plane_numbers[i].first;
        m_plane_numbers[i] = channel_plane_numbers[i].second;
      }

      for (const auto& name_config : configs) {
        // Get the requested processor.
        std::shared_ptr<processor_t> processor = m_factory->create_processor(name_config.first);

        // Configure it.
        processor->configure(name_config.second, m_plane_numbers);

        // If it's the first one, make it the head.
        if (!prev_processor) {
          m_processor_head = processor;
          prev_processor = processor;
          continue;
        }

        // Otherwise, start linking the chain.
        prev_processor->set_next_processor(processor);
        prev_processor = processor;
      }
    }
    
    /** @brief Poll processors for metric, then fill into buffer. */
    virtual void get_pipeline_metrics(const std::unordered_map<tpglibs::MetricKey, signal_t*>& table) {
    
      // get the head of processors
      std::shared_ptr<AbstractProcessor<signal_t>> curr_processor = m_processor_head; 
    
      while (curr_processor){

	curr_processor = curr_processor->get_next_processor();
      
      } 
    }
    
    /**
     * @brief Process a signal through the pipeline.
     */
    virtual std::vector<dunedaq::trgdataformats::TriggerPrimitive> process(const signal_t& signal) {
      signal_t tp_mask = save_state(m_processor_head->process(signal));

      std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps;
      if (check_for_tps(tp_mask))
        tps = generate_tps(tp_mask);

      return tps;
    }

    /** @brief Pure virtual function that will check if any TPs can be generated. */
    virtual bool check_for_tps(const signal_t& tp_mask) = 0;

    /** @brief Pure virtual function that will save the state of the generation. */
    virtual signal_t save_state(const signal_t& processed_signal) = 0;

    /** @brief Pure virtual function that will generate TPs given a mask to draw from. */
    virtual std::vector<dunedaq::trgdataformats::TriggerPrimitive> generate_tps(const signal_t& tp_mask) = 0;

    /** @brief Set the samples over threshold minimum values. */
    virtual void set_sot_minima(const std::vector<uint16_t>& sot_minima) {
      int idx = 0;
      for (auto sot_minimum : sot_minima) {
        m_sot_minima[idx++] = sot_minimum;
      }
    }

  protected:
    /** @brief The on-going ADC integral for channels that are considered active. */
    signal_t m_adc_integral_lo{};
    signal_t m_adc_integral_hi{};
    /** @brief The ADC peak for channels that are considered active. */
    signal_t m_adc_peak{};
    /** @brief The samples over threshold for channels that are considered active. */
    signal_t m_samples_over_threshold{};
    /** @brief The number of samples from `time_start` to the ADC peak. */
    signal_t m_samples_to_peak{};
    /** @brief Detector channel numbers for the 16 channels that are being processed. */
    dunedaq::trgdataformats::channel_t m_channels[16];
    /** @brief Detector plane numbers for the 16 channels that are being processed. */
    int16_t m_plane_numbers[16];
    /** @brief The samples over threshold minimum that a TP from plane `i` must have. */
    uint16_t m_sot_minima[3];
    /** @brief Processor factory singleton. */
    std::shared_ptr<AbstractFactory<processor_t>> m_factory = AbstractFactory<processor_t>::get_instance();
    /** @brief Processor head to start from. */
    std::shared_ptr<processor_t> m_processor_head;
};

} // namespace tpglibs

#endif // TPGLIBS_TPGPIPELINE_HPP_
