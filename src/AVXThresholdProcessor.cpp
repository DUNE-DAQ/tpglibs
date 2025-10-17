/**
 * @file AVXThresholdProcessor.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AVXThresholdProcessor.hpp"

namespace tpglibs {

REGISTER_AVXPROCESSOR_CREATOR("AVXThresholdProcessor", AVXThresholdProcessor)

void AVXThresholdProcessor::configure(const types::tpg_config_map_t& config, const int16_t* plane_numbers) {
  if (config.contains("plane_thresholds")) {
    const std::array<uint16_t, 3> plane_thresholds = config["plane_thresholds"]->get_config_value();
  } else {
    throw MissingProcessorConfig("AVXThresholdProcessor", "plane_thresholds");
  }

  std::array<uint16_t, 16> thresholds;
  for (int i = 0; i < 16; i++) {
    thresholds[i] = plane_thresholds[plane_numbers[i]];
  }

  m_threshold = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(thresholds.data()));
}

__m256i AVXThresholdProcessor::process(const __m256i& signal) {
  __m256i mask = _mm256_cmpgt_epi16(signal, m_threshold);

  // Essentially: mask[i] ? signal[i] : 0.
  __m256i above_threshold = _mm256_blendv_epi8(_mm256_setzero_si256(), signal, mask);
  return AVXProcessor::process(above_threshold);
}

} // namespace tpglibs
