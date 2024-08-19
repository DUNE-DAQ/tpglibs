/**
 * @file AVXRunSumProcessor.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AVXFactory.hpp"
#include "tpglibs/AVXUtils.hpp"

#ifndef TPGENGINE_AVXRUNSUMPROCESSOR_HPP_
#define TPGENGINE_AVXRUNSUMPROCESSOR_HPP_

namespace tpglibs {

class AVXRunSumProcessor : public AVXProcessor {
  __m256i m_memory_factor;
  __m256i m_scale_factor;
  __m256i m_running_sum;

  public:
    __m256i process(const __m256i& signal) override;
    void configure(const nlohmann::json& config, const int16_t* plane_numbers) override;
};

} // namespace tpglibs

#endif // TPGENGINE_AVXRUNSUMPROCESSOR_HPP_
