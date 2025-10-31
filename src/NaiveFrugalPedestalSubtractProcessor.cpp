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
  // Configure common metric collection parameters
  // Register pointers to the ACTUAL member variables, not copies
  // Use shared_ptr with no-op deleter to avoid double-free
  m_internal_state_name_registry.register_internal_state("pedestal", 
    std::shared_ptr<naive_array_t>(&m_pedestal, [](auto*){}));
  m_internal_state_name_registry.register_internal_state("accum", 
    std::shared_ptr<naive_array_t>(&m_accum, [](auto*){}));
    
  configure_internal_state_collection(config);
  
  m_accum_limit = config["accum_limit"];
}

NaiveFrugalPedestalSubtractProcessor::naive_array_t
NaiveFrugalPedestalSubtractProcessor::process(const naive_array_t& signal) {
  // Update sample counter and write internal states to buffer for harvesting
  m_samples++;
  if (m_collect_internal_state_flag && (m_samples % m_sample_period == 0)) {
    m_internal_state_buffer_manager.write_to_active_buffer();
  }

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
