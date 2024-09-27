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

#include "trgdataformats/TriggerPrimitive.hpp"

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
    virtual void configure(const std::vector<std::pair<std::string, nlohmann::json>> configs, const std::vector<std::pair<int16_t, int16_t>> channel_plane_numbers) {
      std::shared_ptr<processor_t> prev_processor = nullptr;

      int16_t plane_numbers[16];
      for (int i = 0; i < 16; i++) {
        m_channels[i] = channel_plane_numbers[i].first;
        plane_numbers[i] = channel_plane_numbers[i].second;
      }

      for (const auto& name_config : configs) {
        // Get the requested processor.
        std::shared_ptr<processor_t> processor = m_factory->create_processor(name_config.first);

        // Configure it.
        processor->configure(name_config.second, plane_numbers);

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

  protected:
    /** @brief The on-going ADC integral for channels that are considered active. */
    signal_t m_adc_integral{};
    /** @brief The ADC peak for channels that are considered active. */
    signal_t m_adc_peak{};
    /** @brief The time over threshold for channels that are considered active. */
    signal_t m_time_over_threshold{};
    /** @brief The time for a channel's ADC peak. */
    signal_t m_time_peak{};
    /** @brief Detector channel numbers for the 16 channels that are being processed. */
    int16_t m_channels[16];
    /** @brief Processor factory singleton. */
    std::shared_ptr<AbstractFactory<processor_t>> m_factory = AbstractFactory<processor_t>::get_instance();
    /** @brief Processor head to start from. */
    std::shared_ptr<processor_t> m_processor_head;
};

} // namespace tpglibs

#endif // TPGLIBS_TPGPIPELINE_HPP_
