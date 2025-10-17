/**
 * @file AbstractProcessor.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_ABSTRACTPROCESSOR_HPP_
#define TPGLIBS_ABSTRACTPROCESSOR_HPP_

#include "tpglibs/ProcessorMetricArray.hpp"
#include "tpglibs/ProcessorInternalStateBufferManager.hpp"
#include "tpglibs/ProcessorInternalStateNameRegistry.hpp"
#include "tpglibs/Types.hpp"

#include <cstdint>
#include <memory>

namespace tpglibs {

/** @brief Abstract signal processor.
 *
 * Configurable signal processor for TPG.
 */
template <class T>
class AbstractProcessor {
  /** @brief Points to next processor in the chain. */
  std::shared_ptr<AbstractProcessor<T>> m_next_processor;

  protected:
    ProcessorInternalStateBufferManager<T> m_internal_state_buffer_manager;
    ProcessorInternalStateNameRegistry<T> m_internal_state_name_registry;
    
    // Sample counting and collection control
    std::atomic<uint64_t> m_samples{0};
    bool m_collect_internal_state_flag{false};
    uint64_t m_sample_period{1};

  public:
    /** @brief Signal type to process on. General __m256i. */
    using signal_type_t = T;

    virtual ~AbstractProcessor() = default;

    ProcessorInternalStateBufferManager<T>* _get_internal_state_buffer_manager() {
      return &m_internal_state_buffer_manager;
    }

    ProcessorInternalStateNameRegistry<T>* _get_internal_state_name_registry() {
      return &m_internal_state_name_registry;
    }

    /** @brief Configure common internal state collection parameters.
     *
     *  This method handles the common configuration for internal state collection
     *  that all processors need. Derived classes should call this method
     *  at the beginning of their configure() implementation.
     *
     *  @param config Map config containing metric_collect_toggle_state,
     *                metric_collect_time_sample_period, and requested_internal_states
     */
    virtual void configure_internal_state_collection(const types::tpg_config_map_t& config) {
      if (config.contains("metric_collect_toggle_state")) {
        m_collect_internal_state_flag = config["metric_collect_toggle_state"]->get_config_value();
      } else {
        m_collect_internal_state_flag = false;
      }

      if (config.contains("metric_collect_time_sample_period")) {
        m_sample_period = config["metric_collect_time_sample_period"]->get_config_value();
      } else {
        m_sample_period = 1;
      }
      
      if (config.contains("requested_internal_states")) {
        m_internal_state_name_registry.parse_requested_internal_state_items(config["requested_internal_states"]->get_config_value());
      } else {
        m_internal_state_name_registry.parse_requested_internal_state_items("");
      }

      m_internal_state_buffer_manager.configure_from_registry(&m_internal_state_name_registry);
    }

    /** @brief Pure virtual function that will configure the processor using plane numbers. */
    virtual void configure(const types::tpg_config_map_t& config, const int16_t* plane_numbers) = 0;

    /** @brief Setter for next processor. */
    void set_next_processor(std::shared_ptr<AbstractProcessor<T>> next_processor) {
      m_next_processor = next_processor;
    }

    /** @brief Getter for next processor. */
    std::shared_ptr<AbstractProcessor<T>> get_next_processor() {
      return m_next_processor;
    }

    /** @brief Simple signal pass-through. */
    virtual T process(const T& signal) {
      if (m_next_processor) {
        return m_next_processor->process(signal);
      }
      return signal;
    }

    /** @brief Get the names of requested internal states (delegates to registry). */
    virtual std::vector<std::string> get_requested_internal_state_names() const {
      return m_internal_state_name_registry.get_names_of_requested_internal_states();
    }

    virtual ProcessorMetricArray<std::array<int16_t, 16>> read_internal_states_as_integer_array() {
      return m_internal_state_buffer_manager.switch_buffer_and_read_casted();
    }
};

} // namespace tpglibs

#endif // TPGLIBS_ABSTRACTPROCESSOR_HPP_
