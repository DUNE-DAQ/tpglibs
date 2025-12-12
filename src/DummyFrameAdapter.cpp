/**
 * @file DummyFrameAdapter.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/testapp/frame/DummyFrameAdapter.hpp"
#include <cstring>
#include <stdexcept>

namespace tpglibs {
namespace testapp {

int16_t DUMMY_FRAME_STRUCT::get_adc(size_t channel, size_t time_sample) const {
  // Calculate which word contains this channel
  const int word_index = channel / 4;
  const int channel_in_word = channel % 4;
  
  // Get the packed word for this time sample
  const size_t word_offset = time_sample * s_words_per_time_sample + word_index;
  const uint64_t packed_word = data[word_offset];
  
  // Extract the 16-bit value from the packed word
  // Channel order in word: 0, 1, 2, 3 at bit positions 0-15, 16-31, 32-47, 48-63
  const uint16_t value_bits = static_cast<uint16_t>((packed_word >> (channel_in_word * 16)) & 0xFFFF);
  
  // Reinterpret as int16_t (preserves sign)
  return static_cast<int16_t>(value_bits);
}

void DUMMY_FRAME_STRUCT::set_adc(size_t channel, size_t time_sample, int16_t value) {
  // Calculate which word contains this channel
  const int word_index = channel / 4;
  const int channel_in_word = channel % 4;
  
  // Get the packed word for this time sample
  const size_t word_offset = time_sample * s_words_per_time_sample + word_index;
  uint64_t& packed_word = data[word_offset];
  
  // Clear the 16-bit slot for this channel
  const uint64_t mask = ~(static_cast<uint64_t>(0xFFFF) << (channel_in_word * 16));
  packed_word &= mask;
  
  // Set the new value (preserve bit pattern by casting through uint16_t)
  const uint16_t value_bits = static_cast<uint16_t>(value);
  packed_word |= (static_cast<uint64_t>(value_bits) << (channel_in_word * 16));
}

std::unique_ptr<DUMMY_FRAME_STRUCT> 
DummyFrameAdapter::create_frame(const RawFrameView& frame_view) {
  // Validate frame view
  if (!frame_view.is_valid()) {
    return nullptr;
  }
  
  // Validate frame view matches expected size (64×256)
  if (!validate_frame_view(frame_view)) {
    return nullptr;
  }
  
  // Create frame object (constructor initializes data and adc_words)
  auto frame = std::make_unique<DUMMY_FRAME_STRUCT>();
  
  // Extract header (first 16 bytes)
  const uint8_t* header_ptr = frame_view.header();
  if (header_ptr == nullptr) {
    return nullptr;
  }
  std::memcpy(&frame->timestamp, header_ptr, sizeof(uint64_t));
  std::memcpy(&frame->another_key, header_ptr + 8, sizeof(uint64_t));
  
  // Extract data (remaining bytes)
  // Binary format stores unpacked int16_t data, we need to pack it into uint64_t words
  const uint8_t* data_ptr = frame_view.data();
  const size_t data_size = frame_view.data_size();
  
  if (data_ptr == nullptr || data_size != calculate_frame_data_size()) {
    return nullptr;
  }
  
  // Read unpacked int16_t data and pack into uint64_t words
  const int16_t* adc_data = reinterpret_cast<const int16_t*>(data_ptr);
  constexpr int words_per_ts = DUMMY_FRAME_STRUCT::s_words_per_time_sample;
  
  for (int t = 0; t < DUMMY_FRAME_STRUCT::s_time_samples_per_frame; ++t) {
    for (int w = 0; w < words_per_ts; ++w) {
      uint64_t packed_word = 0;
      // Pack up to 4 channels into this word
      for (int i = 0; i < 4; ++i) {
        const int channel = w * 4 + i;
        if (channel < DUMMY_FRAME_STRUCT::s_num_channels) {
          const int16_t adc_value = adc_data[t * DUMMY_FRAME_STRUCT::s_num_channels + channel];
          const uint16_t value_bits = static_cast<uint16_t>(adc_value);  // Preserves bit pattern
          packed_word |= (static_cast<uint64_t>(value_bits) << (i * 16));
        }
      }
      frame->data[t * words_per_ts + w] = packed_word;
    }
  }
  
  return frame;
}

size_t DummyFrameAdapter::calculate_frame_size() {
  return FRAME_HEADER_SIZE + calculate_frame_data_size();
}

size_t DummyFrameAdapter::calculate_frame_data_size() {
  // Binary format stores unpacked int16_t: 64 channels × 256 time samples × 2 bytes
  return DUMMY_FRAME_STRUCT::s_num_channels * 
         DUMMY_FRAME_STRUCT::s_time_samples_per_frame * 
         sizeof(int16_t);
}

bool DummyFrameAdapter::validate_frame_view(const RawFrameView& frame_view) {
  if (!frame_view.is_valid()) {
    return false;
  }
  
  const size_t expected_size = calculate_frame_size();
  return frame_view.bytes.size() == expected_size;
}

} // namespace testapp
} // namespace tpglibs

