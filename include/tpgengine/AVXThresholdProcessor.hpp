/**
 * @file AVXThresholdProcessor.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpgengine/AVXFactory.hpp"

#ifndef TPGENGINE_AVXTHRESHOLDPROCESSOR_HPP_
#define TPGENGINE_AVXTHRESHOLDPROCESSOR_HPP_

namespace tpgengine {

class AVXThresholdProcessor : public AVXProcessor {
  __m256i m_threshold;

  public:
    __m256i process(const __m256i& signal) override;
    void configure(const nlohmann::json& config, const int16_t* plane_numbers) override;
};

} // namespace tpgengine

#endif // TPGENGINE_AVXTHRESHOLDPROCESSOR_HPP_
