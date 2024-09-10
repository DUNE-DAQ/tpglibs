/**
 * @file AVXFixedPedestalSubtractProcessor.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AVXFactory.hpp"
#include "tpglibs/AVXFrugalPedestalSubtractProcessor.hpp"

#ifndef TPGLIBS_AVXFIXEDPEDESTALSUBTRACTPROCESSOR_HPP_
#define TPGLIBS_AVXFIXEDPEDESTALSUBTRACTPROCESSOR_HPP_

namespace tpglibs {

class AVXFixedPedestalSubtractProcessor : public AVXFrugalPedestalSubtractProcessor {
  protected:
    uint16_t m_start_period{1000};
    uint16_t m_num_time_steps{0};

  public:
    __m256i process(const __m256i& signal) override;
    void configure(const nlohmann::json& config, const int16_t* plane_numbers) override;
};

} // namespace tpglibs

#endif // TPGLIBS_AVXFIXEDPEDESTALSUBTRACTPROCESSOR_HPP_
