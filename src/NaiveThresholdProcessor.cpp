/**
 * @file NaiveThresholdProcessor.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpgengine/NaiveThresholdProcessor.hpp"

namespace tpgengine {

REGISTER_NAIVEPROCESSOR_CREATOR("NaiveThresholdProcessor", NaiveThresholdProcessor)

void NaiveThresholdProcessor::configure(const nlohmann::json& config, const int16_t* plane_numbers) {
  int16_t config_thresholds[3] = {config["plane0"], config["plane1"], config["plane2"]};

  // Messy. Assumes plane numbers are in {0, 1, 2}.
  for (int i = 0; i < 16; i++) {
    m_threshold[i] = config_thresholds[plane_numbers[i]];
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

} // namespace tpgengine
