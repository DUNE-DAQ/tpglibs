/**
 * @file AVXFrugalPedestalSubtractProcessor.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AVXFrugalPedestalSubtractProcessor.hpp"

namespace tpglibs {

REGISTER_AVXPROCESSOR_CREATOR("AVXFrugalPedestalSubtractProcessor", AVXFrugalPedestalSubtractProcessor)

void AVXFrugalPedestalSubtractProcessor::configure(const nlohmann::json& config, const int16_t* plane_numbers) {
  m_accum_limit = config["accum_limit"];
}

__m256i AVXFrugalPedestalSubtractProcessor::process(const __m256i& signal) {
  // Find the channels that are above or below the pedestal.
  __m256i is_gt = _mm256_cmpgt_epi16(signal, m_pedestal);
  __m256i is_lt = _mm256_cmpgt_epi16(m_pedestal, signal);

  // Update m_accum.
  __m256i to_add = _mm256_setzero_si256();                                    // Assumes equal to pedestal.
  to_add = _mm256_blendv_epi8(to_add, _mm256_set1_epi16(1), is_gt);           // Set the above pedestal case.
  to_add = _mm256_blendv_epi8(to_add, _mm256_set1_epi16(-1), is_lt);          // Set the below pedestal case.

  m_accum = _mm256_add_epi16(m_accum, to_add);

  // Check the accum limit condition.
  is_gt = _mm256_cmpgt_epi16(m_accum, _mm256_set1_epi16(m_accum_limit));      // m_accum > +limit.
  is_lt = _mm256_cmpgt_epi16(_mm256_set1_epi16(-1*m_accum_limit), m_accum);   // m_accum < -limit = -limit > m_accum.

  to_add = _mm256_setzero_si256();
  to_add = _mm256_blendv_epi8(to_add, _mm256_set1_epi16(1), is_gt);
  to_add = _mm256_blendv_epi8(to_add, _mm256_set1_epi16(-1), is_lt);

  // Update pedestal.
  m_pedestal = _mm256_adds_epi16(m_pedestal, to_add);

  // Reset too high/low m_accum channels.
  __m256i need_reset = _mm256_or_si256(is_lt, is_gt);
  m_accum = _mm256_blendv_epi8(m_accum, _mm256_setzero_si256(), need_reset);

  return AVXProcessor::process(_mm256_sub_epi16(signal, m_pedestal));
}

} // namespace tpglibs
