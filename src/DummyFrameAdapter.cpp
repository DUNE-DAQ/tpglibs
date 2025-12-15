/**
 * @file DummyFrameAdapter.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/testapp/frame/DummyFrameAdapter.hpp"
#include <cstring>
#include <iostream>

namespace tpglibs {
namespace testapp {

namespace {
  void warn_out_of_bounds(const char* function_name, size_t channel, size_t time_sample, const char* action) {
    std::cerr << "WARNING: " << function_name << " - Out-of-bounds access: "
              << "channel=" << channel << " (max=" << (DUMMY_FRAME_STRUCT::s_num_channels - 1) << "), "
              << "time_sample=" << time_sample << " (max=" << (DUMMY_FRAME_STRUCT::s_time_samples_per_frame - 1) << "). "
              << action << std::endl;
  }
  
  void warn_adc_clamped(const char* function_name, size_t channel, size_t time_sample, 
                        int16_t original_value, int16_t clamped_value) {
    std::cerr << "WARNING: " << function_name << " - ADC value clamped: "
              << "channel=" << channel << ", time_sample=" << time_sample << ", "
              << "original_value=" << original_value << " -> clamped_value=" << clamped_value << " "
              << "(valid range: [" << DUMMY_FRAME_STRUCT::s_adc_min_value << ", " 
              << DUMMY_FRAME_STRUCT::s_adc_max_value << "])" << std::endl;
  }
}

int16_t DUMMY_FRAME_STRUCT::get_adc(size_t channel, size_t time_sample) const {
  if (channel >= s_num_channels || time_sample >= s_time_samples_per_frame) {
    warn_out_of_bounds("DUMMY_FRAME_STRUCT::get_adc()", channel, time_sample, "Returning safe default (0).");
    return 0;
  }
  
  const int word_index = channel / 4;
  const int channel_in_word = channel % 4;
  const size_t word_offset = time_sample * s_words_per_time_sample + word_index;
  const uint16_t value_bits = static_cast<uint16_t>((data[word_offset] >> (channel_in_word * 16)) & 0xFFFF);
  return static_cast<int16_t>(value_bits);
}

void DUMMY_FRAME_STRUCT::set_adc(size_t channel, size_t time_sample, int16_t value) {
  if (channel >= s_num_channels || time_sample >= s_time_samples_per_frame) {
    warn_out_of_bounds("DUMMY_FRAME_STRUCT::set_adc()", channel, time_sample, "Ignoring operation.");
    return;
  }
  
  const int16_t clamped_value = clamp_adc_value(value);
  if (clamped_value != value) {
    warn_adc_clamped("DUMMY_FRAME_STRUCT::set_adc()", channel, time_sample, value, clamped_value);
  }
  
  const int word_index = channel / 4;
  const int channel_in_word = channel % 4;
  const size_t word_offset = time_sample * s_words_per_time_sample + word_index;
  uint64_t& packed_word = data[word_offset];
  
  const uint64_t mask = ~(static_cast<uint64_t>(0xFFFF) << (channel_in_word * 16));
  packed_word = (packed_word & mask) | (static_cast<uint64_t>(static_cast<uint16_t>(clamped_value)) << (channel_in_word * 16));
}

std::unique_ptr<DUMMY_FRAME_STRUCT> 
DummyFrameAdapter::create_frame(const RawFrameView& frame_view) {
  if (!frame_view.is_valid() || !validate_frame_view(frame_view)) {
    return nullptr;
  }
  
  auto frame = std::make_unique<DUMMY_FRAME_STRUCT>();
  const uint8_t* header_ptr = frame_view.header();
  if (header_ptr == nullptr) {
    return nullptr;
  }
  std::memcpy(&frame->timestamp, header_ptr, sizeof(uint64_t));
  std::memcpy(&frame->another_key, header_ptr + 8, sizeof(uint64_t));
  
  const uint8_t* data_ptr = frame_view.data();
  if (data_ptr == nullptr || frame_view.data_size() != calculate_frame_data_size()) {
    return nullptr;
  }
  
  const int16_t* adc_data = reinterpret_cast<const int16_t*>(data_ptr);
  constexpr int words_per_ts = DUMMY_FRAME_STRUCT::s_words_per_time_sample;
  
  size_t clamp_count = 0;
  size_t first_clamp_channel = 0;
  size_t first_clamp_time_sample = 0;
  int16_t first_clamp_original = 0;
  int16_t first_clamp_clamped = 0;
  
  for (int t = 0; t < DUMMY_FRAME_STRUCT::s_time_samples_per_frame; ++t) {
    for (int w = 0; w < words_per_ts; ++w) {
      uint64_t packed_word = 0;
      for (int i = 0; i < 4; ++i) {
        const int channel = w * 4 + i;
        if (channel < DUMMY_FRAME_STRUCT::s_num_channels) {
          const int16_t adc_value = adc_data[t * DUMMY_FRAME_STRUCT::s_num_channels + channel];
          const int16_t clamped_value = DUMMY_FRAME_STRUCT::clamp_adc_value(adc_value);
          
          if (clamped_value != adc_value && clamp_count++ == 0) {
            first_clamp_channel = channel;
            first_clamp_time_sample = t;
            first_clamp_original = adc_value;
            first_clamp_clamped = clamped_value;
          }
          
          packed_word |= (static_cast<uint64_t>(static_cast<uint16_t>(clamped_value)) << (i * 16));
        }
      }
      frame->data[t * words_per_ts + w] = packed_word;
    }
  }
  
  if (clamp_count > 0) {
    warn_adc_clamped("DummyFrameAdapter::create_frame()", first_clamp_channel, 
                     first_clamp_time_sample, first_clamp_original, first_clamp_clamped);
    if (clamp_count > 1) {
      std::cerr << "  ... and " << (clamp_count - 1) << " more value(s) clamped in this frame." << std::endl;
    }
  }
  
  return frame;
}

size_t DummyFrameAdapter::calculate_frame_size() {
  return FRAME_HEADER_SIZE + calculate_frame_data_size();
}

size_t DummyFrameAdapter::calculate_frame_data_size() {
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

