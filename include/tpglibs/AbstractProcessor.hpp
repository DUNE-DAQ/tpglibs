/**
 * @file AbstractProcessor.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_ABSTRACTPROCESSOR_HPP_
#define TPGLIBS_ABSTRACTPROCESSOR_HPP_

#include <cstdint>
#include <nlohmann/json.hpp>
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

  public:
    /** @brief Signal type to process on. General __m256i. */
    using signal_type_t = T;

    virtual ~AbstractProcessor() = default;

    /** @brief Pure virtual function that will configure the processor using plane numbers. */
    virtual void configure(const nlohmann::json& config, const int16_t* plane_numbers) = 0;

    /** @brief Setter for next processor. */
    void set_next_processor(std::shared_ptr<AbstractProcessor<T>> next_processor) {
      m_next_processor = next_processor;
    }

    /** @brief Simple signal pass-through. */
    virtual T process(const T& signal) {
      if (m_next_processor) {
        return m_next_processor->process(signal);
      }
      return signal;
    }
};

} // namespace tpglibs

#endif // TPGLIBS_ABSTRACTPROCESSOR_HPP_
