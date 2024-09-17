/**
 * @file AVXFrugalPedestalSubtractProcessor.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AVXFactory.hpp"

#ifndef TPGLIBS_AVXFRUGALPEDESTALSUBTRACTPROCESSOR_HPP_
#define TPGLIBS_AVXFRUGALPEDESTALSUBTRACTPROCESSOR_HPP_

namespace tpglibs {

class AVXFrugalPedestalSubtractProcessor : public AVXProcessor {
  protected:
    __m256i m_pedestal = _mm256_set1_epi16(0x4000);  // Set initial start to 14-bit max. Prevents garbage TPs at start.
    __m256i m_accum = _mm256_setzero_si256();
    int16_t m_accum_limit{10};

  public:
    __m256i process(const __m256i& signal) override;
    void configure(const nlohmann::json& config, const int16_t* plane_numbers) override;
};

} // namespace tpglibs

#endif // TPGLIBS_AVXFRUGALPEDESTALSUBTRACTPROCESSOR_HPP_
