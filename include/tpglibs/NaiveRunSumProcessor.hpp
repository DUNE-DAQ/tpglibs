/**
 * @file NaiveRunSumProcessor.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/NaiveFactory.hpp"
#include "tpglibs/NaiveUtils.hpp"

#include <algorithm>

#ifndef TPGLIBS_NAIVERUNSUMPROCESSOR_HPP_
#define TPGLIBS_NAIVERUNSUMPROCESSOR_HPP_

namespace tpglibs {

class NaiveRunSumProcessor : public NaiveProcessor {
  naive_array_t m_memory_factor;
  naive_array_t m_scale_factor;
  naive_array_t m_running_sum;

  public:
    naive_array_t process(const naive_array_t& signal) override;
    void configure(const nlohmann::json& config, const int16_t* plane_numbers) override;
};

} // namespace tpglibs

#endif // TPGLIBS_NAIVERUNSUMPROCESSOR_HPP_
