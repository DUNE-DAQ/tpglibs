/**
 * @file TPComparator.hpp
 *
 * @brief Comparator for TriggerPrimitive objects
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_TESTAPP_TPCOMPARATOR_HPP_
#define TPGLIBS_TESTAPP_TPCOMPARATOR_HPP_

#include "trgdataformats/TriggerPrimitive.hpp"
#include <cstdint>
#include <vector>
#include <string>

namespace tpglibs {
namespace testapp {

/**
 * @brief Result of TP comparison
 */
struct TPComparisonResult {
  bool matches;                    ///< True if TPs match exactly
  size_t first_mismatch_index;     ///< Index of first mismatch (SIZE_MAX if matches)
  std::string mismatch_reason;     ///< Description of mismatch
};

/**
 * @brief Comparator for TriggerPrimitive objects
 *
 * Static utility class for comparing TP vectors. Sorts both vectors
 * using the same key (time_start, channel, samples_over_threshold) before
 * comparing to ensure deterministic results.
 */
class TPComparator {
 public:
  /**
   * @brief Compare two TP vectors
   *
   * Sorts both vectors using the same criteria (time_start, channel, samples_over_threshold)
   * before comparing element-by-element.
   *
   * @param expected Expected TPs (from validation file)
   * @param actual Actual TPs (from TPGenerator)
   * @return TPComparisonResult with match status and first mismatch details
   */
  static TPComparisonResult compare(
      const std::vector<dunedaq::trgdataformats::TriggerPrimitive>& expected,
      const std::vector<dunedaq::trgdataformats::TriggerPrimitive>& actual);
  
  /**
   * @brief Format TP for error messages
   * @param tp TriggerPrimitive to format
   * @return String representation of TP
   */
  static std::string format_tp(const dunedaq::trgdataformats::TriggerPrimitive& tp);

 private:
  /**
   * @brief Sort TPs for deterministic comparison
   *
   * Uses sorting key: time_start (primary), channel (secondary), samples_over_threshold (tertiary).
   * This matches the sorting used by TPWriter for consistency.
   *
   * @param tps Vector of TPs to sort (modified in place)
   */
  static void sort_tps(std::vector<dunedaq::trgdataformats::TriggerPrimitive>& tps);
  
  /**
   * @brief Compare two TPs field-by-field
   * @param expected Expected TP
   * @param actual Actual TP
   * @return Empty string if match, otherwise description of first mismatch
   */
  static std::string compare_single_tp(
      const dunedaq::trgdataformats::TriggerPrimitive& expected,
      const dunedaq::trgdataformats::TriggerPrimitive& actual);
};

} // namespace testapp
} // namespace tpglibs

#include "TPComparator.hxx"

#endif // TPGLIBS_TESTAPP_TPCOMPARATOR_HPP_

