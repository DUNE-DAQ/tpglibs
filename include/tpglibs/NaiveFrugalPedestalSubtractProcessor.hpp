/**
 * @file NaiveFrugalPedestalSubtractProcessor.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/NaiveFactory.hpp"

#ifndef TPGLIBS_NAIVEFRUGALPEDESTALSUBTRACTPROCESSOR_HPP_
#define TPGLIBS_NAIVEFRUGALPEDESTALSUBTRACTPROCESSOR_HPP_

namespace tpglibs {

/** @brief Naive signal processor: Estimates the pedestal and subtracts.
 *
 *  Given a history of signals, this estimates the pedestal by shifting the current estimate when it is wrong in
 *  the same direction m_accum_limit times. For example, if the input signal is greater (less) than the estimated
 *  pedestal 10 (configurable) times in a row, then increment (decrement) the pedestal.
 */
class NaiveFrugalPedestalSubtractProcessor : public NaiveProcessor {
  /** @brief Vector of estimated pedestals for each channel. */
  naive_array_t m_pedestal{};

  /** @brief Vector of counts that a channel's signal was above or below m_pedestal. */
  naive_array_t m_accum{};

  /** @brief Count limit before committing to a pedestal shift. */
  int16_t m_accum_limit{10};

  public:
    /** @brief Estimate the pedestal using the given signal and subtract.
     *
     *  @param signal A vector of channel signals.
     *  @return The input signal minus the estimated pedestal.
     */
    naive_array_t process(const naive_array_t& signal) override;

    /** @brief Configure the accumulation limit according to plane number.
     *
     *  @param config JSON config for the accumulation limits per plane.
     *  @param plane_numbers Array of plane numbers. Gives the channels to apply the accumulation limit.
     */
    void configure(const nlohmann::json& config, const int16_t* plane_numbers) override;
};

} // namespace tpglibs

#endif // TPGLIBS_NAIVEFRUGALPEDESTALSUBTRACTPROCESSOR_HPP_
