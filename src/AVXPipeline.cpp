/**
 * @file AVXPipeline.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "tpglibs/AVXPipeline.hpp"

namespace tpglibs {

__m256i
AVXPipeline::save_state(const __m256i& processed_signal) {
  __m256i active       = _mm256_cmpgt_epi16(processed_signal, _mm256_setzero_si256());
  __m256i inactive     = _mm256_cmpeq_epi16(processed_signal, _mm256_setzero_si256());
  __m256i was_inactive = _mm256_cmpeq_epi16(m_samples_over_threshold, _mm256_setzero_si256());

  // If it was *not* inactive and is now inactive, then it must be a new TP.
  __m256i new_tps = _mm256_andnot_si256(was_inactive, inactive);

  // Get the potentially saturated integral and overflown integral.
  __m256i adc_integral_sat = _mm256_adds_epu16(m_adc_integral_lo, processed_signal);
  m_adc_integral_lo = _mm256_add_epi16(m_adc_integral_lo, processed_signal);

  // If it is saturated, then increment the hi. The overflown integral already "reset".
  __m256i is_saturated = _mm256_cmpeq_epi16(adc_integral_sat, m_max_value_register);
  // If lo and sat are the same, then it is *not* saturated and happened to exactly sum to 0xFFFF.
  __m256i exact = _mm256_cmpeq_epi16(m_adc_integral_lo, adc_integral_sat);
  // So, (!exact) & is_saturated == [truly saturated].
  is_saturated  = _mm256_andnot_si256(exact, is_saturated);

  __m256i to_add = _mm256_and_si256(m_ones_register, is_saturated);
  m_adc_integral_hi = _mm256_adds_epu16(m_adc_integral_hi, to_add);

  __m256i above_peak = _mm256_cmpgt_epi16(processed_signal, m_adc_peak);

  m_adc_peak = _mm256_max_epi16(m_adc_peak, processed_signal);
  m_samples_to_peak = _mm256_blendv_epi8(m_samples_to_peak, m_samples_over_threshold, above_peak);

  __m256i time_add = _mm256_blendv_epi8(_mm256_setzero_si256(), m_ones_register, active);
  m_samples_over_threshold = _mm256_adds_epi16(m_samples_over_threshold, time_add);

  return new_tps;
}

bool
AVXPipeline::check_for_tps(const __m256i& tp_mask) {
  // tp_mask & 0xFFFF = 0 -> tp_mask == 0.
  // True => tp_mask is all zeros and has no TPs.
  // Negate!
  return !_mm256_testz_si256(tp_mask, _mm256_set1_epi16(-1));
}

void
AVXPipeline::generate_tps(const __m256i& tp_mask) {
  // Mask everything that's relevant.
  __m256i samples_over_threshold = _mm256_blendv_epi8(_mm256_setzero_si256(), m_samples_over_threshold, tp_mask);
  __m256i adc_integral_lo = _mm256_blendv_epi8(_mm256_setzero_si256(), m_adc_integral_lo, tp_mask);
  __m256i adc_integral_hi = _mm256_blendv_epi8(_mm256_setzero_si256(), m_adc_integral_hi, tp_mask);
  __m256i adc_peak = _mm256_blendv_epi8(_mm256_setzero_si256(), m_adc_peak, tp_mask);
  __m256i samples_to_peak = _mm256_blendv_epi8(_mm256_setzero_si256(), m_samples_to_peak, tp_mask);

  // Convert to uint16_t.
  uint16_t tp_sot[16], tp_integral_lo[16], tp_integral_hi[16], tp_adc_peak[16], tp_samples_to_peak[16];
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_sot), samples_over_threshold);
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_integral_lo), adc_integral_lo);
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_integral_hi), adc_integral_hi);
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_adc_peak), adc_peak);
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_samples_to_peak), samples_to_peak);

  //std::vector<TriggerPrimitive> tps;
  for (int i = 0; i < 16; i++) {
    if (tp_sot[i] < m_sot_minima[m_plane_numbers[i]]) continue;  // Don't track short TPs.
    TriggerPrimitive tp;
    tp.adc_integral        = uint32_t(tp_integral_lo[i]) + (uint32_t(tp_integral_hi[i]) << 16);
    tp.adc_peak            = tp_adc_peak[i];
    tp.channel             = m_channels[i];
    tp.samples_to_peak     = tp_samples_to_peak[i];
    tp.samples_over_threshold = tp_sot[i];

    // time_start is handled at the next level up, since it is aware of the true and relative times.
    //tps.push_back(tp);
  }

  // Reset the channels that generated tps.
  m_samples_over_threshold = _mm256_blendv_epi8(m_samples_over_threshold, _mm256_setzero_si256(), tp_mask);
  m_adc_integral_lo     = _mm256_blendv_epi8(m_adc_integral_lo, _mm256_setzero_si256(), tp_mask);
  m_adc_integral_hi     = _mm256_blendv_epi8(m_adc_integral_hi, _mm256_setzero_si256(), tp_mask);
  m_adc_peak            = _mm256_blendv_epi8(m_adc_peak, _mm256_setzero_si256(), tp_mask);
  m_samples_to_peak     = _mm256_blendv_epi8(m_samples_to_peak, _mm256_setzero_si256(), tp_mask);

  // Finalize.
  //return tps;
}

} // namespace tpglibs
