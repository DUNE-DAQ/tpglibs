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
 * Simple frame structure matching the binary format:
 * - Header: 16 bytes (8 bytes timestamp + 8 bytes another_key)
 * - Data: num_channels * num_time_samples * sizeof(int16_t) bytes
 * - Data layout: row-major order [time_sample][channel]
 */
struct DUMMY_FRAME_STRUCT {
  uint64_t timestamp;      ///< Frame timestamp (8 bytes)
  uint64_t another_key;    ///< Additional header field (8 bytes)
  std::vector<int16_t> data;  ///< Frame data: row-major [time_sample][channel]
  
  size_t num_channels;     ///< Number of channels in this frame
  size_t num_time_samples; ///< Number of time samples in this frame
  
  /**
   * @brief Get ADC value for a specific channel and time sample
   * @param channel Channel index [0, num_channels-1]
   * @param time_sample Time sample index [0, num_time_samples-1]
   * @return ADC value
   */
  int16_t get_adc(size_t channel, size_t time_sample) const {
    return data[time_sample * num_channels + channel];
  }
  
  /**
   * @brief Set ADC value for a specific channel and time sample
   * @param channel Channel index [0, num_channels-1]
   * @param time_sample Time sample index [0, num_time_samples-1]
   * @param value ADC value to set
   */
  void set_adc(size_t channel, size_t time_sample, int16_t value) {
    data[time_sample * num_channels + channel] = value;
  }
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
   * @param num_channels Number of channels in the frame
   * @param num_time_samples Number of time samples in the frame
   * @return Unique pointer to DUMMY_FRAME_STRUCT, or nullptr if conversion fails
   */
  static std::unique_ptr<DUMMY_FRAME_STRUCT> 
  create_frame(const RawFrameView& frame_view, 
               size_t num_channels, 
               size_t num_time_samples);
  
  /**
   * @brief Calculate frame size from dimensions
   * @param num_channels Number of channels
   * @param num_time_samples Number of time samples
   * @return Total frame size in bytes (header + data)
   */
  static size_t calculate_frame_size(size_t num_channels, 
                                     size_t num_time_samples);
  
  /**
   * @brief Calculate frame data size (excluding header)
   * @param num_channels Number of channels
   * @param num_time_samples Number of time samples
   * @return Frame data size in bytes
   */
  static size_t calculate_frame_data_size(size_t num_channels,
                                          size_t num_time_samples);
  
  /**
   * @brief Validate frame dimensions
   * @param num_channels Number of channels
   * @param num_time_samples Number of time samples
   * @return true if dimensions are valid (both > 0)
   */
  static bool validate_dimensions(size_t num_channels, 
                                  size_t num_time_samples);
  
  /**
   * @brief Validate raw frame view matches expected dimensions
   * @param frame_view Raw frame view to validate
   * @param num_channels Expected number of channels
   * @param num_time_samples Expected number of time samples
   * @return true if frame view matches expected dimensions
   */
  static bool validate_frame_view(const RawFrameView& frame_view,
                                  size_t num_channels,
                                  size_t num_time_samples);

 private:
  static constexpr size_t FRAME_HEADER_SIZE = 16;  // 8 bytes timestamp + 8 bytes another_key
};

} // namespace testapp
} // namespace tpglibs

#include "DummyFrameAdapter.hxx"

#endif // TPGLIBS_TESTAPP_DUMMYFRAMEADAPTER_HPP_



