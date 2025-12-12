/**
 * @file TPComparator.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/testapp/tp/TPComparator.hpp"
#include <algorithm>
#include <sstream>
#include <limits>

namespace tpglibs {
namespace testapp {

TPComparisonResult TPComparator::compare(
    const std::vector<dunedaq::trgdataformats::TriggerPrimitive>& expected,
    const std::vector<dunedaq::trgdataformats::TriggerPrimitive>& actual) {
  
  TPComparisonResult result;
  result.matches = false;
  result.first_mismatch_index = std::numeric_limits<size_t>::max();
  result.mismatch_reason = "";
  
  // Make copies for sorting (don't modify input vectors)
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected_sorted = expected;
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual_sorted = actual;
  
  // Sort both vectors using same criteria
  sort_tps(expected_sorted);
  sort_tps(actual_sorted);
  
  // Check size mismatch
  if (expected_sorted.size() != actual_sorted.size()) {
    result.first_mismatch_index = 0;
    std::ostringstream oss;
    oss << "Size mismatch: expected " << expected_sorted.size() 
        << " TPs, got " << actual_sorted.size() << " TPs";
    result.mismatch_reason = oss.str();
    return result;
  }
  
  // Compare element-by-element
  for (size_t i = 0; i < expected_sorted.size(); ++i) {
    std::string mismatch = compare_single_tp(expected_sorted[i], actual_sorted[i]);
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

std::string TPComparator::format_tp(const dunedaq::trgdataformats::TriggerPrimitive& tp) {
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

void TPComparator::sort_tps(std::vector<dunedaq::trgdataformats::TriggerPrimitive>& tps) {
  std::sort(tps.begin(), tps.end(), 
    [](const dunedaq::trgdataformats::TriggerPrimitive& a,
       const dunedaq::trgdataformats::TriggerPrimitive& b) {
      // Primary: time_start
      if (a.time_start != b.time_start) {
        return a.time_start < b.time_start;
      }
      // Secondary: channel
      if (a.channel != b.channel) {
        return a.channel < b.channel;
      }
      // Tertiary: samples_over_threshold
      return a.samples_over_threshold < b.samples_over_threshold;
    });
}

std::string TPComparator::compare_single_tp(
    const dunedaq::trgdataformats::TriggerPrimitive& expected,
    const dunedaq::trgdataformats::TriggerPrimitive& actual) {
  
  // Compare all fields exactly (order matches sorting key priority for consistency)
  if (expected.time_start != actual.time_start) {
    std::ostringstream oss;
    oss << "time_start mismatch: expected " << expected.time_start 
        << ", got " << actual.time_start;
    return oss.str();
  }
  
  if (expected.channel != actual.channel) {
    std::ostringstream oss;
    oss << "channel mismatch: expected " << expected.channel 
        << ", got " << actual.channel;
    return oss.str();
  }
  
  if (expected.adc_peak != actual.adc_peak) {
    std::ostringstream oss;
    oss << "adc_peak mismatch: expected " << expected.adc_peak 
        << ", got " << actual.adc_peak;
    return oss.str();
  }
  
  if (expected.samples_over_threshold != actual.samples_over_threshold) {
    std::ostringstream oss;
    oss << "samples_over_threshold mismatch: expected " << expected.samples_over_threshold 
        << ", got " << actual.samples_over_threshold;
    return oss.str();
  }
  
  if (expected.adc_integral != actual.adc_integral) {
    std::ostringstream oss;
    oss << "adc_integral mismatch: expected " << expected.adc_integral 
        << ", got " << actual.adc_integral;
    return oss.str();
  }
  
  if (expected.samples_to_peak != actual.samples_to_peak) {
    std::ostringstream oss;
    oss << "samples_to_peak mismatch: expected " << expected.samples_to_peak 
        << ", got " << actual.samples_to_peak;
    return oss.str();
  }
  
  // All fields match
  return "";
}

} // namespace testapp
} // namespace tpglibs

