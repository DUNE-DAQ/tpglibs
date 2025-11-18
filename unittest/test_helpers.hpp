/**
 * @file test_helpers.hpp
 *
 * @brief Common test helper functions for tpglibs unit tests
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_UNITTEST_TEST_HELPERS_HPP_
#define TPGLIBS_UNITTEST_TEST_HELPERS_HPP_

#include "trgdataformats/TriggerPrimitive.hpp"
#include <fstream>
#include <vector>
#include <cstdint>

namespace tpglibs {
namespace unittest {

/**
 * @brief Write binary file header (magic, version, reserved)
 * @param file Output file stream
 */
inline void write_file_header(std::ofstream& file) {
  uint32_t magic = 0x54504754;  // "TPGT"
  uint32_t version = 0x010004;  // 1.0.4
  uint32_t reserved = 0x00000000;
  file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
  file.write(reinterpret_cast<const char*>(&version), sizeof(version));
  file.write(reinterpret_cast<const char*>(&reserved), sizeof(reserved));
}

/**
 * @brief Write frame (16-byte header + data) with uint8_t data
 * @param file Output file stream
 * @param timestamp Frame timestamp (8 bytes)
 * @param another_key Additional header field (8 bytes)
 * @param data Frame data as vector of uint8_t
 */
inline void write_frame(std::ofstream& file, 
                        uint64_t timestamp, 
                        uint64_t another_key, 
                        const std::vector<uint8_t>& data) {
  file.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
  file.write(reinterpret_cast<const char*>(&another_key), sizeof(another_key));
  file.write(reinterpret_cast<const char*>(data.data()), data.size());
}

/**
 * @brief Write frame (16-byte header + data) with int16_t data
 * @param file Output file stream
 * @param timestamp Frame timestamp (8 bytes)
 * @param another_key Additional header field (8 bytes)
 * @param data Frame data as vector of int16_t
 */
inline void write_frame(std::ofstream& file, 
                        uint64_t timestamp, 
                        uint64_t another_key, 
                        const std::vector<int16_t>& data) {
  file.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
  file.write(reinterpret_cast<const char*>(&another_key), sizeof(another_key));
  file.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(int16_t));
}

/**
 * @brief Write TP record (frame_index, num_tps, TPs)
 * @param file Output file stream
 * @param frame_index Frame index this TP set belongs to
 * @param tps Vector of TriggerPrimitive objects
 */
inline void write_tp_record(std::ofstream& file, 
                            uint32_t frame_index,
                            const std::vector<dunedaq::trgdataformats::TriggerPrimitive>& tps) {
  uint32_t num_tps = static_cast<uint32_t>(tps.size());
  file.write(reinterpret_cast<const char*>(&frame_index), sizeof(frame_index));
  file.write(reinterpret_cast<const char*>(&num_tps), sizeof(num_tps));
  
  if (!tps.empty()) {
    file.write(reinterpret_cast<const char*>(tps.data()), 
               tps.size() * sizeof(dunedaq::trgdataformats::TriggerPrimitive));
  }
}

/**
 * @brief Create a simple TriggerPrimitive for testing
 * @param time_start Timestamp when TP starts
 * @param channel Channel number
 * @param adc_peak Peak ADC value
 * @param samples_over_threshold Number of samples over threshold
 * @return TriggerPrimitive with specified values
 */
inline dunedaq::trgdataformats::TriggerPrimitive create_test_tp(int64_t time_start,
                                                                 uint32_t channel,
                                                                 int16_t adc_peak,
                                                                 uint16_t samples_over_threshold) {
  dunedaq::trgdataformats::TriggerPrimitive tp;
  tp.time_start = time_start;
  tp.channel = channel;
  tp.adc_peak = adc_peak;
  tp.samples_over_threshold = samples_over_threshold;
  // Initialize other fields to default values
  tp.adc_integral = 0;
  tp.samples_to_peak = 0;
  return tp;
}

} // namespace unittest
} // namespace tpglibs

#endif // TPGLIBS_UNITTEST_TEST_HELPERS_HPP_

