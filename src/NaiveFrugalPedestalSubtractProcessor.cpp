/**
 * @file NaiveFrugalPedestalSubtractProcessor.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/NaiveFrugalPedestalSubtractProcessor.hpp"

namespace tpglibs {

REGISTER_NAIVEPROCESSOR_CREATOR("NaiveFrugalPedestalSubtractProcessor", NaiveFrugalPedestalSubtractProcessor)

void NaiveFrugalPedestalSubtractProcessor::configure(const nlohmann::json& config, const int16_t* plane_numbers) {
  m_accum_limit = config["accum_limit"];
}

NaiveFrugalPedestalSubtractProcessor::naive_array_t
NaiveFrugalPedestalSubtractProcessor::process(const naive_array_t& signal) {
  naive_array_t subtracted_signal;
  for (int i = 0; i < 16; i++) {
    // Increment if above.
    if (signal[i] > m_pedestal[i])
      m_accum[i]++;

    // Decrement if below.
    if (signal[i] < m_pedestal[i])
      m_accum[i]--;

    // Increment pedestal if we've hit the top limit.
    if (m_accum[i] > m_accum_limit) {
      m_pedestal[i]++;
      m_accum[i] = 0;
    }

    // Decrement pedestal if we've hit the low limit.
    if (m_accum[i] < -1*m_accum_limit) {
      m_pedestal[i]--;
      m_accum[i] = 0;
    }

    subtracted_signal[i] = signal[i] - m_pedestal[i];
  }

  return NaiveProcessor::process(subtracted_signal);
}

} // namespace tpglibs
