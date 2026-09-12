/**
 * @file TPValidator.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/testapp/tp/TPValidator.hpp"
#include <algorithm>
#include <sstream>
#include <limits>
#include <tuple>

namespace tpglibs {
namespace testapp {

TPValidationResult TPValidator::validate(
    const std::vector<dunedaq::trgdataformats::TriggerPrimitive>& expected,
    const std::vector<dunedaq::trgdataformats::TriggerPrimitive>& actual) {
  
  TPValidationResult result;
  result.matches = false;
  result.first_mismatch_index = std::numeric_limits<size_t>::max();
  result.mismatch_reason = "";
  
  // Check size mismatch before sorting (early return for efficiency)
  if (expected.size() != actual.size()) {
    result.first_mismatch_index = 0;
    std::ostringstream oss;
    oss << "Size mismatch: expected " << expected.size() 
        << " TPs, got " << actual.size() << " TPs";
    result.mismatch_reason = oss.str();
    return result;
  }
  
  // Make copies for sorting (don't modify input vectors)
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected_sorted = expected;
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual_sorted = actual;
  
  // Sort both vectors using same criteria
  sort_tps(expected_sorted);
  sort_tps(actual_sorted);
  
  // Compare element-by-element
  for (size_t i = 0; i < expected_sorted.size(); ++i) {
    std::string mismatch = compare(expected_sorted[i], actual_sorted[i]);
    if (!mismatch.empty()) {
      result.first_mismatch_index = i;
      result.mismatch_reason = mismatch;
      return result;
    }
  }
  
  // All TPs match
  result.matches = true;
  return result;
}

std::string TPValidator::format_tp(const dunedaq::trgdataformats::TriggerPrimitive& tp) {
  std::ostringstream oss;
  oss << "TP(time_start=" << tp.time_start
      << ", channel=" << tp.channel
      << ", adc_peak=" << tp.adc_peak
      << ", samples_over_threshold=" << tp.samples_over_threshold
      << ", adc_integral=" << tp.adc_integral
      << ", samples_to_peak=" << tp.samples_to_peak
      << ")";
  return oss.str();
}

void TPValidator::sort_tps(std::vector<dunedaq::trgdataformats::TriggerPrimitive>& tps) {
  std::sort(tps.begin(), tps.end(), 
    [](const dunedaq::trgdataformats::TriggerPrimitive& a,
       const dunedaq::trgdataformats::TriggerPrimitive& b) {
      // Lexicographic comparison: time_start (primary), channel (secondary), samples_over_threshold (tertiary)
      return std::tie(a.time_start, a.channel, a.samples_over_threshold) < 
             std::tie(b.time_start, b.channel, b.samples_over_threshold);
    });
}

std::string TPValidator::compare(
    const dunedaq::trgdataformats::TriggerPrimitive& expected,
    const dunedaq::trgdataformats::TriggerPrimitive& actual) {
  
  // Compare all fields exactly (order matches sorting key priority for consistency)
  // Collect all mismatches to provide comprehensive diagnostic information
  std::ostringstream oss;
  bool has_mismatch = false;
  
  if (expected.time_start != actual.time_start) {
    if (has_mismatch) oss << "; ";
    oss << "time_start mismatch: expected " << expected.time_start 
        << ", got " << actual.time_start;
    has_mismatch = true;
  }
  
  if (expected.channel != actual.channel) {
    if (has_mismatch) oss << "; ";
    oss << "channel mismatch: expected " << expected.channel 
        << ", got " << actual.channel;
    has_mismatch = true;
  }
  
  if (expected.adc_peak != actual.adc_peak) {
    if (has_mismatch) oss << "; ";
    oss << "adc_peak mismatch: expected " << expected.adc_peak 
        << ", got " << actual.adc_peak;
    has_mismatch = true;
  }
  
  if (expected.samples_over_threshold != actual.samples_over_threshold) {
    if (has_mismatch) oss << "; ";
    oss << "samples_over_threshold mismatch: expected " << expected.samples_over_threshold 
        << ", got " << actual.samples_over_threshold;
    has_mismatch = true;
  }
  
  if (expected.adc_integral != actual.adc_integral) {
    if (has_mismatch) oss << "; ";
    oss << "adc_integral mismatch: expected " << expected.adc_integral 
        << ", got " << actual.adc_integral;
    has_mismatch = true;
  }
  
  if (expected.samples_to_peak != actual.samples_to_peak) {
    if (has_mismatch) oss << "; ";
    oss << "samples_to_peak mismatch: expected " << expected.samples_to_peak 
        << ", got " << actual.samples_to_peak;
    has_mismatch = true;
  }
  
  // Return combined mismatch message or empty string if all fields match
  return oss.str();
}

} // namespace testapp
} // namespace tpglibs

