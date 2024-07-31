/**
 * @file AVXRunSumProcessor.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpgengine/AVXRunSumProcessor.hpp"

namespace tpgengine {

void AVXRunSumProcessor::configure(const nlohmann::json& config, const int16_t* plane_numbers) {
  int16_t memory_factors[16];
  int16_t config_memory[3] = {config["memory_factor_plane0"],
                              config["memory_factor_plane1"],
                              config["memory_factor_plane2"]};
  int16_t scale_factors[16];
  int16_t config_scale[3]  = {config["scale_factor_plane0"],
                              config["scale_factor_plane1"],
                              config["scale_factor_plane2"]};

  for (int i = 0; i < 16; i++) {
    memory_factors[i] = config_memory[plane_numbers[i]];
    scale_factors[i] = config_scale[plane_numbers[i]];
  }

  m_memory_factor = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(memory_factors));
  m_scale_factor = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(scale_factors));
}

__m256i AVXRunSumProcessor::process(const __m256i& signal) {
  __m256i scaled_rs = _mm256_div_epi16(m_running_sum, 10);
  scaled_rs = _mm256_mullo_epi16(scaled_rs, m_memory_factor);

  __m256i scaled_signal = _mm256_div_epi16(signal, 10);
  scaled_signal = _mm256_mullo_epi16(scaled_signal, m_scale_factor);

  m_running_sum = _mm256_adds_epi16(scaled_rs, scaled_signal);
  return AVXProcessor::process(m_running_sum);
}

} // namespace tpgengine
