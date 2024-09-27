/**
 * @file AVXFixedPedestalSubtractProcessor.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AVXFactory.hpp"
#include "tpglibs/AVXFrugalPedestalSubtractProcessor.hpp"

#ifndef TPGLIBS_AVXFIXEDPEDESTALSUBTRACTPROCESSOR_HPP_
#define TPGLIBS_AVXFIXEDPEDESTALSUBTRACTPROCESSOR_HPP_

namespace tpglibs {

/** @brief AVX signal processor: Fixes the estimated pedestal after a configured amount of time.
 *
 *  Look to AVXFrugalPedestalSubtractProcessor for details on how the pedestal is estimated.
 */
class AVXFixedPedestalSubtractProcessor : public AVXFrugalPedestalSubtractProcessor {
  protected:
    /** @brief Configured number of samples to use before fixing. */
    uint16_t m_start_period{1000};

    /** @brief Counted number of samples. */
    uint16_t m_num_time_steps{0};

  public:
    /** @brief Estimates the pedestal for some time and subtracts. */
    __m256i process(const __m256i& signal) override;

    /** @brief Configure the number of startup samples to use before fixing.
     *
     *  @param config JSON config for start period per plane.
     *  @param plane_numbers Array of plane numbers. Gives the channels to apply the start period.
     */
    void configure(const nlohmann::json& config, const int16_t* plane_numbers) override;
};

} // namespace tpglibs

#endif // TPGLIBS_AVXFIXEDPEDESTALSUBTRACTPROCESSOR_HPP_
