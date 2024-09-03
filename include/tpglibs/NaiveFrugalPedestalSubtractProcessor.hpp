/**
 * @file NaiveFrugalPedestalSubtractProcessor.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/NaiveFactory.hpp"

#ifndef TPGLIBS_NAIVEFRUGALPEDESTALSUBTRACTPROCESSOR_HPP_
#define TPGLIBS_NAIVEFRUGALPEDESTALSUBTRACTPROCESSOR_HPP_

namespace tpglibs {

class NaiveFrugalPedestalSubtractProcessor : public NaiveProcessor {
  naive_array_t m_pedestal{};
  naive_array_t m_accum{};
  int16_t m_accum_limit{10};

  public:
    naive_array_t process(const naive_array_t& signal) override;
    void configure(const nlohmann::json& config, const int16_t* plane_numbers) override;
};

} // namespace tpglibs

#endif // TPGLIBS_NAIVEFRUGALPEDESTALSUBTRACTPROCESSOR_HPP_
