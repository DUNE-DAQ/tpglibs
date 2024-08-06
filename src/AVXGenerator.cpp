/**
 * @file AVXGenerator.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "tpgengine/AVXGenerator.hpp"

namespace tpgengine {

__m256i
AVXGenerator::save_state(const __m256i& processed_signal) {
  __m256i active     = _mm256_cmpgt_epi16(processed_signal, _mm256_setzero_si256());
  __m256i inactive   = _mm256_cmpeq_epi16(processed_signal, _mm256_setzero_si256());
  __m256i was_active = _mm256_cmpgt_epi16(m_adc_integral, _mm256_setzero_si256());

  // If it was active and is now inactive, then it must be a new TP.
  __m256i new_tps    = _mm256_and_si256(was_active, inactive);

  m_adc_integral = _mm256_adds_epi16(m_adc_integral, processed_signal);

  __m256i above_peak = _mm256_cmpgt_epi16(processed_signal, m_adc_peak);

  m_adc_peak = _mm256_max_epi16(m_adc_peak, processed_signal);
  m_time_peak = _mm256_blendv_epi8(m_time_peak, m_time_over_threshold, above_peak);

  __m256i time_add = _mm256_blendv_epi8(_mm256_setzero_si256(), _mm256_set1_epi16(1), active);
  m_time_over_threshold = _mm256_adds_epi16(m_time_over_threshold, time_add);

  return new_tps;
}

bool
AVXGenerator::check_for_tps(const __m256i& tp_mask) {
  // tp_mask & 0xFFFF = 0 -> tp_mask == 0.
  // True => tp_mask is all zeros and has no TPs.
  // Negate!
  return !_mm256_testz_si256(tp_mask, _mm256_set1_epi16(0xFFFF));
}

std::vector<dunedaq::trgdataformats::TriggerPrimitive>
AVXGenerator::generate_tps(const __m256i& tp_mask) {
  // Mask everything that's relevant.
  __m256i time_over_threshold = _mm256_blendv_epi8(_mm256_setzero_si256(), m_time_over_threshold, tp_mask);
  __m256i adc_integral = _mm256_blendv_epi8(_mm256_setzero_si256(), m_adc_integral, tp_mask);
  __m256i adc_peak = _mm256_blendv_epi8(_mm256_setzero_si256(), m_adc_peak, tp_mask);
  __m256i time_peak = _mm256_blendv_epi8(_mm256_setzero_si256(), m_time_peak, tp_mask);

  // Convert to int16_t.
  int16_t tp_tot[16], tp_integral[16], tp_adc_peak[16], tp_time_peak[16];
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_tot), time_over_threshold);
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_integral), adc_integral);
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_adc_peak), adc_peak);
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_time_peak), time_peak);

  std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps;
  for (int i = 0; i < 16; i++) {
    if (tp_tot[i] == 0) continue;  // Don't track non-TPs.
    dunedaq::trgdataformats::TriggerPrimitive tp;
    tp.adc_integral        = tp_integral[i];
    tp.adc_peak            = tp_adc_peak[i];
    tp.time_peak           = tp_time_peak[i];
    tp.time_over_threshold = tp_tot[i];
    tp.type                = tp.Type::kTPC;
    tp.algorithm           = tp.Algorithm::kUnknown; // TODO: Choose a separate way to represent the mixed processors.

    // Outside dependencies and references are needed for these. Let the outside handle their updates.
    tp.channel = i;
    tp.time_start = tp_tot[i];
    tps.push_back(tp);
  }

  // Reset the channels that generated tps.
  m_time_over_threshold = _mm256_blendv_epi8(m_time_over_threshold, _mm256_setzero_si256(), tp_mask);
  m_adc_integral        = _mm256_blendv_epi8(m_adc_integral, _mm256_setzero_si256(), tp_mask);
  m_adc_peak            = _mm256_blendv_epi8(m_adc_peak, _mm256_setzero_si256(), tp_mask);
  m_time_peak           = _mm256_blendv_epi8(m_time_peak, _mm256_setzero_si256(), tp_mask);

  // Finalize.
  return tps;
}

} // namespace tpgengine
