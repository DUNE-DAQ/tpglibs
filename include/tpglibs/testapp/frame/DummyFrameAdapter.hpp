/**
 * @file DummyFrameAdapter.hpp
 *
 * @brief Adapter to convert raw frame view to DUMMY_FRAME_STRUCT
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_TESTAPP_DUMMYFRAMEADAPTER_HPP_
#define TPGLIBS_TESTAPP_DUMMYFRAMEADAPTER_HPP_

#include "tpglibs/testapp/frame/FrameReader.hpp"
#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>

namespace tpglibs {
namespace testapp {

/**
 * @brief Dummy frame structure for test applications
 *
 * Frame structure compatible with TPGenerator (64 channels, 256 time samples):
 * - Header: 8 bytes (timestamp)
 * - Data: 256 time samples × 16 words × 8 bytes = 32,768 bytes (packed uint64_t format)
 * - Data layout: packed uint64_t words, each word contains 4 consecutive 16-bit ADC values
 * - ADC VALUE RANGE: Values must be in 14-bit range [0, 16383] to match TPGenerator expectations
 * - Storage: Values stored as 16-bit int16_t, but constrained to 14-bit range
 * - Compatible with TPGenerator's expected interface
 */
struct DUMMY_FRAME_STRUCT {
  // Since this is dedicated test application, hardcoded for 64 channels, 256 time samples
  using word_t = uint64_t;
  static constexpr int s_num_channels = 64;
  static constexpr int s_time_samples_per_frame = 256;
  static constexpr int s_bits_per_adc = 16;  // Layout bits: controls array dimension and register alignment
  static constexpr int s_words_per_time_sample = 16;
  
  // ADC value bit depth constants (separate from layout bits)
  static constexpr int s_adc_value_bits = 14;  // Actual ADC value bit depth
  static constexpr int16_t s_adc_max_value = 0x3FFF;  // 16383 - 14-bit max
  static constexpr int16_t s_adc_min_value = 0;       // 0 - 14-bit min
  
  uint64_t timestamp;      ///< Frame timestamp (8 bytes)
  
  // Data storage: packed uint64_t words (256 time samples × 16 words = 4096 words)
  // Each uint64_t word contains 4 consecutive 16-bit ADC values
  // Layout: data[t * 16 + w] where t is time sample, w is word index
  std::vector<word_t> data;
  
  // TPGenerator interface: pointer to array of s_bits_per_adc elements
  const word_t (*adc_words)[s_bits_per_adc];
  
  /**
   * @brief Get timestamp (TPGenerator interface)
   * @return Frame timestamp
   */
  uint64_t get_timestamp() const { return timestamp; }
  
  /**
   * @brief Clamp ADC value to 14-bit range [0, 16383]
   * @param value ADC value to clamp
   * @return Clamped value in range [s_adc_min_value, s_adc_max_value]
   */
  static constexpr int16_t clamp_adc_value(int16_t value) {
    return (value > s_adc_max_value) ? s_adc_max_value :
           (value < s_adc_min_value) ? s_adc_min_value : value;
  }
  
  /**
   * @brief Get ADC value for a specific channel and time sample
   * @param channel Channel index [0, 63]
   * @param time_sample Time sample index [0, 255]
   * @return ADC value (returns 0 for out-of-bounds access with warning)
   * @note Out-of-bounds access will emit a warning to stderr and return 0
   */
  int16_t get_adc(size_t channel, size_t time_sample) const;
  
  /**
   * @brief Set ADC value for a specific channel and time sample
   * @param channel Channel index [0, 63]
   * @param time_sample Time sample index [0, 255]
   * @param value ADC value to set (will be clamped to [0, 16383] if out of range)
   * @note Out-of-bounds access will emit a warning to stderr and be ignored
   * @note Values exceeding 14-bit range [0, 16383] will be clamped with warning
   */
  void set_adc(size_t channel, size_t time_sample, int16_t value);
  
  /**
   * @brief Constructor - initializes data storage and adc_words pointer
   */
  DUMMY_FRAME_STRUCT() 
    : data(s_time_samples_per_frame * s_words_per_time_sample),
      adc_words(reinterpret_cast<const word_t (*)[s_bits_per_adc]>(data.data())) {}
};

/**
 * @brief Adapter to convert raw frame view to DUMMY_FRAME_STRUCT
 *
 * Static utility class that converts RawFrameView from FrameReader into
 * typed DUMMY_FRAME_STRUCT objects. This keeps FrameReader frame-type
 * agnostic.
 */
class DummyFrameAdapter {
 public:
  /**
   * @brief Create frame from raw frame view
   * @param frame_view Raw frame view from FrameReader
   * @return Unique pointer to DUMMY_FRAME_STRUCT, or nullptr if conversion fails
   */
  static std::unique_ptr<DUMMY_FRAME_STRUCT> 
  create_frame(const RawFrameView& frame_view);
  
  /**
   * @brief Calculate frame size (fixed for 64×256)
   * @return Total frame size in bytes (header + data)
   */
  static size_t calculate_frame_size();
  
  /**
   * @brief Calculate frame data size (fixed for 64×256)
   * @return Frame data size in bytes
   */
  static size_t calculate_frame_data_size();
  
  /**
   * @brief Validate raw frame view matches expected size (64×256)
   * @param frame_view Raw frame view to validate
   * @return true if frame view matches expected size
   */
  static bool validate_frame_view(const RawFrameView& frame_view);

 private:
  static constexpr size_t FRAME_HEADER_SIZE = 8;  // 8 bytes timestamp
};

} // namespace testapp
} // namespace tpglibs

#endif // TPGLIBS_TESTAPP_DUMMYFRAMEADAPTER_HPP_



