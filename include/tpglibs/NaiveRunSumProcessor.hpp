/**
 * @file NaiveRunSumProcessor.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/NaiveFactory.hpp"
#include "tpglibs/NaiveUtils.hpp"

#include <algorithm>

#ifndef TPGLIBS_NAIVERUNSUMPROCESSOR_HPP_
#define TPGLIBS_NAIVERUNSUMPROCESSOR_HPP_

namespace tpglibs {

/** @brief Naive signal processor: Calculates the running sum of the signal.
 *
 *  Calculates the running sum of the signal with some factors. This tries to model the following equation:
 *  `RS = R * RS + S * signal`, where
 *  * `RS` is the on-going running sum,
 *  * `R` is the memory factor of the on-going running sum,
 *  * `S` is the scale factor for the incoming signal to avoid overflowing,
 *  * `signal` is the incoming signal.
 *  Division is intentionally unorthodox to match what is possible in AVX.
 */
class NaiveRunSumProcessor : public NaiveProcessor {
  /** @brief The `R` factor in the model equation. */
  naive_array_t m_memory_factor;

  /** @brief The `S` factor in the model equation. */
  naive_array_t m_scale_factor;

  /** @brief The `RS` in the model equation. */
  naive_array_t m_running_sum;

  public:
    /** @brief Calculate and store the running sum.
     *
     *  @param signal The input signal to process on.
     *  @return The calculated running sum.
     */
    naive_array_t process(const naive_array_t& signal) override;

    /** @brief Configures the `R` factor and `S` factor according to plane.
     *
     *  @param config JSON of the `R` and `S` factors to use per plane.
     *  @param plane_numbers Array of plane numbers. Gives the channels to apply the `R` and `S` factors.
     */
    void configure(const nlohmann::json& config, const int16_t* plane_numbers) override;
};

} // namespace tpglibs

#endif // TPGLIBS_NAIVERUNSUMPROCESSOR_HPP_
