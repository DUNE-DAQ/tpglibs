/**
 * @file AVXFrugalPedestalSubtractProcessor.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AVXFactory.hpp"

#ifndef TPGLIBS_AVXFRUGALPEDESTALSUBTRACTPROCESSOR_HPP_
#define TPGLIBS_AVXFRUGALPEDESTALSUBTRACTPROCESSOR_HPP_

namespace tpglibs {

/** @brief AVX signal processor: Estimates the pedestal and subtracts.
 *
 *  Given a history of signals, this estimates the pedestal by shifting the current estimate when it is wrong in
 *  the same direction m_accum_limit times. For example, if the input signal is greater (less) than the estimated
 *  pedestal 10 (configurable) times in a row, then increment (decrement) the pedestal.
 */
class AVXFrugalPedestalSubtractProcessor : public AVXProcessor {
  protected:
    /** @brief Vector of estimated pedestals for each channel. */
    __m256i m_pedestal = _mm256_set1_epi16(0x4000);  // Set initial start to 14-bit max. Prevents garbage TPs at start.

    /** @brief Vector of counts that a channel's signal was above or below m_pedestal. */
    __m256i m_accum = _mm256_setzero_si256();

    /** @brief Count limit before committing to a pedestal shift. */
    int16_t m_accum_limit{10};

  public:
    /** @brief Estimate the pedestal using the given signal and subtract.
     *
     *  @param signal A vector of channel signals.
     *  @return The input signal minus the estimated pedestal.
     */
    __m256i process(const __m256i& signal) override;

    /** @brief Configure the accumulation limit according to plane number.
     *
     *  @param config JSON config for the accumulation limits per plane.
     *  @param plane_numbers Array of plane numbers. Gives the channels to apply the accumulation limit.
     */
    void configure(const nlohmann::json& config, const int16_t* plane_numbers) override;
};

} // namespace tpglibs

#endif // TPGLIBS_AVXFRUGALPEDESTALSUBTRACTPROCESSOR_HPP_
