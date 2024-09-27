/**
 * @file AVXThresholdProcessor.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AVXFactory.hpp"

#ifndef TPGLIBS_AVXTHRESHOLDPROCESSOR_HPP_
#define TPGLIBS_AVXTHRESHOLDPROCESSOR_HPP_

namespace tpglibs {

/** @brief AVX signal processor: Passes signals above a threshold. */
class AVXThresholdProcessor : public AVXProcessor {
  /** @brief Vector of thresholds to apply. */
  __m256i m_threshold;

  public:
    /** @brief Masks channels with signals below threshold.
     *
     *  @param signal Vector of channel signals to process.
     *  @return A vector of channel signals. Signals that were below threshold are set to 0.
     */
    __m256i process(const __m256i& signal) override;

    /** @brief Configures thresholds according to plane numbers.
     *
     *  @param config JSON containing thresholds for the 3 planes.
     *  @param plane_numbers Array of plane numbers. Gives the channels to apply the appropriate thresholds.
     */
    void configure(const nlohmann::json& config, const int16_t* plane_numbers) override;
};

} // namespace tpglibs

#endif // TPGLIBS_AVXTHRESHOLDPROCESSOR_HPP_
