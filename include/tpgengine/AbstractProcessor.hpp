/**
 * @file AbstractProcessor.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGENGINE_ABSTRACTPROCESSOR_HPP_
#define TPGENGINE_ABSTRACTPROCESSOR_HPP_

#include <cstdint>
#include <nlohmann/json.hpp>
#include <memory>

namespace tpgengine {

template <class T>
class AbstractProcessor {
  std::shared_ptr<AbstractProcessor<T>> m_next_processor;

  public:
    using signal_type_t = T;

    virtual ~AbstractProcessor() = default;

    virtual void configure(const nlohmann::json& config, const int16_t* plane_numbers) = 0;
    void set_next_processor(std::shared_ptr<AbstractProcessor<T>> next_processor) {
      m_next_processor = next_processor;
    }

    virtual T process(const T& signal) {
      if (m_next_processor) {
        return m_next_processor->process(signal);
      }
      return signal;
    }
};

} // namespace tpgengine

#endif // TPGENGINE_ABSTRACTPROCESSOR_HPP_
