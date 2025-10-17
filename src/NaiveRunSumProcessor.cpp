/**
 * @file NaiveRunSumProcessor.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/NaiveRunSumProcessor.hpp"

namespace tpglibs {

REGISTER_NAIVEPROCESSOR_CREATOR("NaiveRunSumProcessor", NaiveRunSumProcessor)

void NaiveRunSumProcessor::configure(const types::tpg_config_map_t& config, const int16_t* plane_numbers) {
  if (config.contains("plane_memory_factors")) {
    const std::array<int, 3> plane_memory_factor = config.at("plane_memory_factors")->get_config_value();
  } else {
    throw MissingProcessorConfig("NaiveRunSumProcessor", "plane_memory_factors");
  }

  if (config.contains("plane_scale_factors")) {
    const std::array<int, 3> plane_scale_factor = config.at("plane_scale_factors")->get_config_value();
  } else {
    throw MissingProcessorConfig("NaiveRunSumProcessor", "plane_scale_factors");
  }

  for (int i = 0; i < 16; i++) {
    m_memory_factor[i] = plane_memory_factor[plane_numbers[i]];
    m_scale_factor[i] = plane_scale_factor[plane_numbers[i]];
  }
}

NaiveRunSumProcessor::naive_array_t NaiveRunSumProcessor::process(const naive_array_t& signal) {
  for (int i = 0; i < 16; i++) {
    int32_t scaled_rs = _naive_div_int16(m_running_sum[i], 10);
    scaled_rs *= m_memory_factor[i];

    int32_t scaled_signal = _naive_div_int16(signal[i], 10);
    scaled_signal *= m_scale_factor[i];

    int32_t intermediate = scaled_signal + scaled_rs;
    m_running_sum[i] = std::min(intermediate, INT16_MAX);
  }
  return NaiveProcessor::process(m_running_sum);
}

} // namespace tpglibs
