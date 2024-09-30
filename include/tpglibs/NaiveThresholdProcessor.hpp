/**
 * @file NaiveThresholdProcessor.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/NaiveFactory.hpp"

#ifndef TPGLIBS_NAIVETHRESHOLDPROCESSOR_HPP_
#define TPGLIBS_NAIVETHRESHOLDPROCESSOR_HPP_

namespace tpglibs {

/** @brief Naive signal processor: Passes signals above a threshold. */
class NaiveThresholdProcessor : public NaiveProcessor {
  /** @brief Vector of thresholds to apply. */
  naive_array_t m_threshold;

  public:
    /** @brief Masks channels with signals below threshold.
     *
     *  @param signal Vector of channel signals to process.
     *  @return A vector of channel signals. Signals that were below threshold are set to 0.
     */
    naive_array_t process(const naive_array_t& signal) override;

    /** @brief Configures thresholds according to plane numbers.
     *
     *  @param config JSON containing thresholds for the 3 planes.
     *  @param plane_numbers Array of plane numbers. Gives the channels to apply the appropriate thresholds.
     */
    void configure(const nlohmann::json& config, const int16_t* plane_numbers) override;
};

} // namespace tpglibs

#endif // TPGLIBS_NAIVETHRESHOLDPROCESSOR_HPP_
