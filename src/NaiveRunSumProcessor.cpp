/**
 * @file NaiveRunSumProcessor.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpgengine/NaiveRunSumProcessor.hpp"

namespace tpgengine {

REGISTER_NAIVEPROCESSOR_CREATOR("NaiveRunSumProcessor", NaiveRunSumProcessor)

void NaiveRunSumProcessor::configure(const nlohmann::json& config, const int16_t* plane_numbers) {
  int16_t config_memory[3] = {config["memory_factor_plane0"],
                              config["memory_factor_plane1"],
                              config["memory_factor_plane2"]};
  int16_t config_scale[3]  = {config["scale_factor_plane0"],
                              config["scale_factor_plane1"],
                              config["scale_factor_plane2"]};

  for (int i = 0; i < 16; i++) {
    m_memory_factor[i] = config_memory[plane_numbers[i]];
    m_scale_factor[i] = config_scale[plane_numbers[i]];
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

} // namespace tpgengine
