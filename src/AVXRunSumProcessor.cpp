/**
 * @file AVXRunSumProcessor.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AVXRunSumProcessor.hpp"

namespace tpglibs {

REGISTER_AVXPROCESSOR_CREATOR("AVXRunSumProcessor", AVXRunSumProcessor)

void AVXRunSumProcessor::configure(const types::tpg_config_map_t& config, const int16_t* plane_numbers) {
  if (config.contains("plane_memory_factors")) {
    const std::array<uint16_t, 3> plane_memory_factors = config["plane_memory_factors"]->get_config_value();
  } else {
    throw MissingProcessorConfig("AVXRunSumProcessor", "plane_memory_factors");
  }

  if (config.contains("plane_scale_factors")) {
    const std::array<uint16_t, 3> plane_scale_factors = config["plane_scale_factors"]->get_config_value();
  } else {
    throw MissingProcessorConfig("AVXRunSumProcessor", "plane_scale_factors");
  }

  std::array<uint16_t, 16> memory_factors;
  std::array<uint16_t, 16> scale_factors;

  for (int i = 0; i < 16; i++) {
    memory_factors[i] = plane_memory_factors[plane_numbers[i]];
    scale_factors[i] = plane_scale_factors[plane_numbers[i]];
  }


  m_memory_factor = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(memory_factors.data()));
  m_scale_factor = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(scale_factors.data()));
}

__m256i AVXRunSumProcessor::process(const __m256i& signal) {
  __m256i scaled_rs = _mm256_div_epi16(m_running_sum, 10);
  scaled_rs = _mm256_mullo_epi16(scaled_rs, m_memory_factor);

  __m256i scaled_signal = _mm256_div_epi16(signal, 10);
  scaled_signal = _mm256_mullo_epi16(scaled_signal, m_scale_factor);

  m_running_sum = _mm256_adds_epi16(scaled_rs, scaled_signal);
  return AVXProcessor::process(m_running_sum);
}

} // namespace tpglibs
