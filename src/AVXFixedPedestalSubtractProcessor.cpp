/**
 * @file AVXFixedPedestalSubtractProcessor.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AVXFixedPedestalSubtractProcessor.hpp"

namespace tpglibs {

REGISTER_AVXPROCESSOR_CREATOR("AVXFixedPedestalSubtractProcessor", AVXFixedPedestalSubtractProcessor)

void AVXFixedPedestalSubtractProcessor::configure(const types::tpg_config_map_t& config, const int16_t* plane_numbers) {
  AVXFrugalPedestalSubtractProcessor::configure(config, plane_numbers);

  if (config.contains("start_period")) {
    m_start_period = config["start_period"]->get_config_value();
  } else {
    throw MissingProcessorConfig("AVXFixedPedestalSubtractProcessor", "start_period");
  }
}

__m256i AVXFixedPedestalSubtractProcessor::process(const __m256i& signal) {
  if (m_num_time_steps < m_start_period) {
    m_num_time_steps++;
    return AVXFrugalPedestalSubtractProcessor::process(signal);
  }
  return AVXProcessor::process(_mm256_sub_epi16(signal, m_pedestal));
}

} // namespace tpglibs
