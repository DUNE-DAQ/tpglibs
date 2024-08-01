/**
 * @file AVXThresholdProcessor.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpgengine/AVXThresholdProcessor.hpp"

namespace tpgengine {

REGISTER_AVXPROCESSOR_CREATOR("AVXThresholdProcessor", AVXThresholdProcessor)

void AVXThresholdProcessor::configure(const nlohmann::json& config, const int16_t* plane_numbers) {
  int16_t thresholds[16];
  int16_t config_thresholds[3] = {config["plane0"], config["plane1"], config["plane2"]};

  // Messy. Assumes plane numbers are in {0, 1, 2}.
  for (int i = 0; i < 16; i++) {
    thresholds[i] = config_thresholds[plane_numbers[i]];
  }

  m_threshold = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(thresholds));
}

__m256i AVXThresholdProcessor::process(const __m256i& signal) {
  __m256i mask = _mm256_cmpgt_epi16(signal, m_threshold);

  // Essentially: mask[i] ? signal[i] : 0.
  __m256i above_threshold = _mm256_blendv_epi8(_mm256_setzero_si256(), signal, mask);
  return AVXProcessor::process(above_threshold);
}

} // namespace tpgengine
