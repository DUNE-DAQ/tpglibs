/**
 * @file NaiveThresholdProcessor.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpgengine/NaiveProcessor.hpp"

#ifndef TPGENGINE_NAIVETHRESHOLDPROCESSOR_HPP_
#define TPGENGINE_NAIVETHRESHOLDPROCESSOR_HPP_

namespace tpgengine {

class NaiveThresholdProcessor : public NaiveProcessor {
  naive_array_t m_threshold;

  public:
    naive_array_t process(const naive_array_t& signal) override;
    void configure(const nlohmann::json& config, const int16_t* plane_numbers) override;
};

} // namespace tpgengine

#endif // TPGENGINE_NAIVETHRESHOLDPROCESSOR_HPP_
