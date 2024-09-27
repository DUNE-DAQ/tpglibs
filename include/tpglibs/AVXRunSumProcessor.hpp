/**
 * @file AVXRunSumProcessor.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AVXFactory.hpp"
#include "tpglibs/AVXUtils.hpp"

#ifndef TPGLIBS_AVXRUNSUMPROCESSOR_HPP_
#define TPGLIBS_AVXRUNSUMPROCESSOR_HPP_

namespace tpglibs {

/** @brief AVX signal processor: Calculates the running sum of the signal.
 *
 *  Calculates the running sum of the signal with some factors. This tries to model the following equation:
 *  `RS = R * RS + S * signal`, where
 *  * `RS` is the on-going running sum,
 *  * `R` is the memory factor of the on-going running sum,
 *  * `S` is the scale factor for the incoming signal to avoid overflowing,
 *  * `signal` is the incoming signal.
 */
class AVXRunSumProcessor : public AVXProcessor {
  /** @brief The `R` factor in the model equation. */
  __m256i m_memory_factor;

  /** @brief The `S` factor in the model equation. */
  __m256i m_scale_factor;

  /** @brief The `RS` in the model equation. */
  __m256i m_running_sum;

  public:
    /** @brief Calculate and store the running sum.
     *
     *  @param signal The input signal to process on.
     *  @return The calculated running sum.
     */
    __m256i process(const __m256i& signal) override;

    /** @brief Configures the `R` factor and `S` factor according to plane.
     *
     *  @param config JSON of the `R` and `S` factors to use per plane.
     *  @param plane_numbers Array of plane numbers. Gives the channels to apply the `R` and `S` factors.
     */
    void configure(const nlohmann::json& config, const int16_t* plane_numbers) override;
};

} // namespace tpglibs

#endif // TPGLIBS_AVXRUNSUMPROCESSOR_HPP_
