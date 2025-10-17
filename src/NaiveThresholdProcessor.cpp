/**
 * @file NaiveThresholdProcessor.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/NaiveThresholdProcessor.hpp"

namespace tpglibs {

REGISTER_NAIVEPROCESSOR_CREATOR("NaiveThresholdProcessor", NaiveThresholdProcessor)

void NaiveThresholdProcessor::configure(const types::tpg_config_map_t& config, const int16_t* plane_numbers) {
  if (config.contains("plane_thresholds")) {
    std::array<int, 3> plane_thresholds = config["plane_thresholds"]->get_config_value();
  } else {
    throw MissingProcessorConfig("NaiveThresholdProcessor", "plane_thresholds");
  }

  // Messy. Assumes plane numbers are in {0, 1, 2}.
  for (int i = 0; i < 16; i++) {
    m_threshold[i] = plane_thresholds[plane_numbers[i]];
  }
}

NaiveThresholdProcessor::naive_array_t NaiveThresholdProcessor::process(const naive_array_t& signal) {
  naive_array_t above_threshold;
  for (int i = 0; i < 16; i++) {
    if (signal[i] > m_threshold[i])
      above_threshold[i] = signal[i];
  }
  return NaiveProcessor::process(above_threshold);
}

} // namespace tpglibs
