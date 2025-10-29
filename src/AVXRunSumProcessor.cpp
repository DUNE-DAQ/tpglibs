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

void AVXRunSumProcessor::configure(const nlohmann::json& config, const int16_t* plane_numbers) {
  // Configure common metric collection parameters
  // Register pointers to the ACTUAL member variables, not copies
  // Use shared_ptr with no-op deleter to avoid double-free
  m_internal_state_name_registry.register_internal_state("r", 
    std::shared_ptr<__m256i>(&m_memory_factor, [](auto*){}));
  m_internal_state_name_registry.register_internal_state("s", 
    std::shared_ptr<__m256i>(&m_scale_factor, [](auto*){}));
  m_internal_state_name_registry.register_internal_state("rs", 
    std::shared_ptr<__m256i>(&m_running_sum, [](auto*){}));
    
  configure_internal_state_collection(config);

  int16_t memory_factors[16];
  int16_t plane_memory_factors[3] = {config["memory_factor_plane0"],
                                     config["memory_factor_plane1"],
                                     config["memory_factor_plane2"]};
  int16_t memory_divisors[16];
  int16_t plane_memory_divisors[3] = {config["memory_divisor_plane0"],
                                      config["memory_divisor_plane1"],
                                      config["memory_divisor_plane2"]};
  int16_t scale_factors[16];
  int16_t plane_scale_factors[3]  = {config["scale_factor_plane0"],
                                     config["scale_factor_plane1"],
                                     config["scale_factor_plane2"]};
  int16_t scale_divisors[16];
  int16_t plane_scale_divisors[3] = {config["scale_divisor_plane0"],
                                     config["scale_divisor_plane1"],
                                     config["scale_divisor_plane2"]};

  for (int i = 0; i < 16; i++) {
    memory_factors[i] = plane_memory_factors[plane_numbers[i]];
    memory_divisors[i] = 0x7FFF / plane_memory_divisors[plane_numbers[i]];  // Need to adjust for AVX2 usage.
    scale_factors[i] = plane_scale_factors[plane_numbers[i]];
    scale_divisors[i] = 0x7FFF / plane_scale_divisors[plane_numbers[i]];  // Need to adjust for AVX2 usage.
  }

  m_memory_factor = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(memory_factors));
  m_memory_divisor = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(memory_divisors));
  m_scale_factor = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(scale_factors));
  m_scale_divisor = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(scale_divisors));
}

__m256i AVXRunSumProcessor::process(const __m256i& signal) {
  // Update sample counter and write internal states to buffer for harvesting
  m_samples++;
  if (m_collect_internal_state_flag && (m_samples % m_sample_period == 0)) {
    m_internal_state_buffer_manager.write_to_active_buffer();
  }

  __m256i scaled_rs = _mm256_mulhrs_epi16(m_running_sum, m_memory_divisor);
  scaled_rs = _mm256_mullo_epi16(scaled_rs, m_memory_factor);

  __m256i scaled_signal = _mm256_mulhrs_epi16(signal, m_scale_divisor);
  scaled_signal = _mm256_mullo_epi16(scaled_signal, m_scale_factor);

  m_running_sum = _mm256_adds_epi16(scaled_rs, scaled_signal);
  return AVXProcessor::process(m_running_sum);
}

} // namespace tpglibs
