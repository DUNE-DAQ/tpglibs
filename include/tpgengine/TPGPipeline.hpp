/**
 * @file TPGPipeline.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGENGINE_TPGPIPELINE_HPP_
#define TPGENGINE_TPGPIPELINE_HPP_

#include "tpgengine/AbstractFactory.hpp"

#include "trgdataformats/TriggerPrimitive.hpp"

#include <nlohmann/json.hpp>
#include <vector>

namespace tpgengine {

template <typename T, typename U>
class TPGPipeline {
  public:
    using processor_t = T;
    using signal_t = U;

    virtual void configure(const std::vector<nlohmann::json> configs, const std::vector<std::pair<int16_t, int16_t>> channel_plane_numbers) {
      std::shared_ptr<processor_t> prev_processor = nullptr;

      int16_t plane_numbers[16];
      for (int i = 0; i < 16; i++) {
        m_channels[i] = channel_plane_numbers[i].first;
        plane_numbers[i] = channel_plane_numbers[i].second;
      }

      for (const nlohmann::json& config : configs) {
        // Get the requested processor.
        std::shared_ptr<processor_t> processor = m_factory->create_processor(config["name"]);

        // Configure it.
        processor->configure(config["config"], plane_numbers);

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

    virtual std::vector<dunedaq::trgdataformats::TriggerPrimitive> process(const signal_t& signal) {
      signal_t tp_mask = save_state(m_processor_head->process(signal));

      std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps;
      if (check_for_tps(tp_mask))
        tps = generate_tps(tp_mask);

      return tps;
    }

    virtual bool check_for_tps(const signal_t& tp_mask) = 0;
    virtual signal_t save_state(const signal_t& processed_signal) = 0;
    virtual std::vector<dunedaq::trgdataformats::TriggerPrimitive> generate_tps(const signal_t& tp_mask) = 0;

  protected:
    signal_t m_adc_integral{};
    signal_t m_adc_peak{};
    signal_t m_time_over_threshold{};
    signal_t m_time_peak{};
    int16_t m_channels[16];
    std::shared_ptr<AbstractFactory<processor_t>> m_factory = AbstractFactory<processor_t>::get_instance();
    std::shared_ptr<processor_t> m_processor_head;
};

} // namespace tpgengine

#endif // TPGENGINE_TPGPIPELINE_HPP_
