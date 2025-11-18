/**
 * @file DummyFrameAdapter.hxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_TESTAPP_DUMMYFRAMEADAPTER_HXX_
#define TPGLIBS_TESTAPP_DUMMYFRAMEADAPTER_HXX_

#include "DummyFrameAdapter.hpp"
#include <cstring>
#include <stdexcept>

namespace tpglibs {
namespace testapp {

std::unique_ptr<DUMMY_FRAME_STRUCT> 
DummyFrameAdapter::create_frame(const RawFrameView& frame_view,
                                 size_t num_channels,
                                 size_t num_time_samples) {
  // Validate dimensions
  if (!validate_dimensions(num_channels, num_time_samples)) {
    return nullptr;
  }
  
  // Validate frame view
  if (!frame_view.is_valid()) {
    return nullptr;
  }
  
  // Validate frame view matches expected size
  if (!validate_frame_view(frame_view, num_channels, num_time_samples)) {
    return nullptr;
  }
  
  // Create frame object
  auto frame = std::make_unique<DUMMY_FRAME_STRUCT>();
  frame->num_channels = num_channels;
  frame->num_time_samples = num_time_samples;
  
  // Extract header (first 16 bytes)
  const uint8_t* header_ptr = frame_view.header();
  if (header_ptr == nullptr) {
    return nullptr;  // Invalid frame view
  }
  std::memcpy(&frame->timestamp, header_ptr, sizeof(uint64_t));
  std::memcpy(&frame->another_key, header_ptr + 8, sizeof(uint64_t));
  
  // Extract data (remaining bytes)
  const size_t expected_data_size = calculate_frame_data_size(num_channels, num_time_samples);
  const uint8_t* data_ptr = frame_view.data();
  const size_t data_size = frame_view.data_size();
  
  // Additional safety check: validate_frame_view already checked size, but verify pointers are valid
  if (data_ptr == nullptr || data_size != expected_data_size) {
    return nullptr;  // Size mismatch or invalid pointer
  }
  
  // Copy data (row-major order: [time_sample][channel])
  frame->data.resize(num_channels * num_time_samples);
  std::memcpy(frame->data.data(), data_ptr, data_size);
  
  return frame;
}

size_t DummyFrameAdapter::calculate_frame_size(size_t num_channels,
                                                size_t num_time_samples) {
  return FRAME_HEADER_SIZE + calculate_frame_data_size(num_channels, num_time_samples);
}

size_t DummyFrameAdapter::calculate_frame_data_size(size_t num_channels,
                                                     size_t num_time_samples) {
  return num_channels * num_time_samples * sizeof(int16_t);
}

bool DummyFrameAdapter::validate_dimensions(size_t num_channels,
                                             size_t num_time_samples) {
  return (num_channels > 0) && (num_time_samples > 0);
}

bool DummyFrameAdapter::validate_frame_view(const RawFrameView& frame_view,
                                            size_t num_channels,
                                            size_t num_time_samples) {
  if (!frame_view.is_valid()) {
    return false;
  }
  
  const size_t expected_size = calculate_frame_size(num_channels, num_time_samples);
  return frame_view.bytes.size() == expected_size;
}

} // namespace testapp
} // namespace tpglibs

#endif // TPGLIBS_TESTAPP_DUMMYFRAMEADAPTER_HXX_


