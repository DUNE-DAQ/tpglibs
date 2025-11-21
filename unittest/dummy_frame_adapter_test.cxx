/**
 * @file dummy_frame_adapter_test.cxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE dummy_frame_adapter_test
#define FMT_HEADER_ONLY

#include "tpglibs/testapp/frame/DummyFrameAdapter.hpp"
#include "tpglibs/testapp/frame/FrameReader.hpp"
#include "test_helpers.hpp"

#include <boost/test/unit_test.hpp>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>

using tpglibs::unittest::write_file_header;
using tpglibs::unittest::write_frame;

BOOST_AUTO_TEST_SUITE(DummyFrameAdapterTest)

BOOST_AUTO_TEST_CASE(TestCreateFrameValid)
{
  // Create test file with one frame (64 channels, 256 time samples - fixed dimensions)
  std::string temp_filename = "/tmp/test_dummy_adapter.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  uint64_t timestamp = 1234567890;
  uint64_t another_key = 9876543210;
  
  // Create test data: 64 channels, 256 time samples
  // Fill with test pattern: value = (time_sample * 100 + channel) % 1000
  constexpr size_t num_channels = 64;
  constexpr size_t num_time_samples = 256;
  std::vector<int16_t> frame_data(num_channels * num_time_samples);
  for (size_t t = 0; t < num_time_samples; ++t) {
    for (size_t c = 0; c < num_channels; ++c) {
      frame_data[t * num_channels + c] = static_cast<int16_t>((t * 100 + c) % 1000);
    }
  }
  
  write_frame(temp_file, timestamp, another_key, frame_data);
  temp_file.close();
  
  // Read frame using FrameReader (fixed size, no parameters needed)
  tpglibs::testapp::FrameReader reader(temp_filename);
  auto [status, frame_view] = reader.next_frame();
  
  BOOST_CHECK(status == tpglibs::testapp::FrameReadStatus::OK);
  BOOST_CHECK(frame_view.is_valid());
  
  // Convert to DUMMY_FRAME_STRUCT using adapter (no dimension parameters)
  auto frame = tpglibs::testapp::DummyFrameAdapter::create_frame(frame_view);
  
  BOOST_CHECK(frame != nullptr);
  BOOST_CHECK_EQUAL(frame->timestamp, timestamp);
  BOOST_CHECK_EQUAL(frame->another_key, another_key);
  // Data is stored as packed uint64_t words: 256 * 16 = 4096 words
  BOOST_CHECK_EQUAL(frame->data.size(), num_time_samples * 16);
  
  // Verify data content via get_adc (unpacks from uint64_t words)
  BOOST_CHECK_EQUAL(frame->get_adc(0, 0), 0);
  BOOST_CHECK_EQUAL(frame->get_adc(1, 0), 1);
  BOOST_CHECK_EQUAL(frame->get_adc(0, 1), 100);
  BOOST_CHECK_EQUAL(frame->get_adc(63, 255), static_cast<int16_t>((255 * 100 + 63) % 1000));
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestCreateFrameInvalidSize)
{
  // Create test file with wrong size (not 64×256)
  std::string temp_filename = "/tmp/test_dummy_adapter_invalid.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  uint64_t timestamp = 1234;
  uint64_t another_key = 5678;
  std::vector<int16_t> frame_data(12, 0x42);  // Wrong size (should be 64*256 = 16384)
  
  write_frame(temp_file, timestamp, another_key, frame_data);
  temp_file.close();
  
  // Read frame (will fail because size doesn't match fixed 64×256)
  tpglibs::testapp::FrameReader reader(temp_filename);
  auto [status, frame_view] = reader.next_frame();
  // FrameReader will fail because frame size doesn't match expected 64×256
  BOOST_CHECK(status == tpglibs::testapp::FrameReadStatus::ERROR);
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestCreateFrameInvalidView)
{
  // Create invalid frame view (empty)
  tpglibs::testapp::RawFrameView invalid_view;
  invalid_view.bytes.clear();  // Empty view
  
  auto frame = tpglibs::testapp::DummyFrameAdapter::create_frame(invalid_view);
  BOOST_CHECK(frame == nullptr);
}

BOOST_AUTO_TEST_CASE(TestCalculateFrameSize)
{
  // Test frame size calculation (fixed 64×256)
  size_t frame_size = tpglibs::testapp::DummyFrameAdapter::calculate_frame_size();
  // Header: 16 bytes, Data: 64*256*2 = 32768 bytes, Total: 32784 bytes
  BOOST_CHECK_EQUAL(frame_size, 16 + 32768);
  
  size_t frame_data_size = tpglibs::testapp::DummyFrameAdapter::calculate_frame_data_size();
  BOOST_CHECK_EQUAL(frame_data_size, 32768);
}

BOOST_AUTO_TEST_CASE(TestValidateFrameView)
{
  // Create test file with one frame (64×256)
  std::string temp_filename = "/tmp/test_dummy_adapter_validate.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  uint64_t timestamp = 1234;
  uint64_t another_key = 5678;
  constexpr size_t num_channels = 64;
  constexpr size_t num_time_samples = 256;
  std::vector<int16_t> frame_data(num_channels * num_time_samples, 0x42);
  
  write_frame(temp_file, timestamp, another_key, frame_data);
  temp_file.close();
  
  // Read frame
  tpglibs::testapp::FrameReader reader(temp_filename);
  auto [status, frame_view] = reader.next_frame();
  BOOST_CHECK(status == tpglibs::testapp::FrameReadStatus::OK);
  
  // Validate with correct size (64×256)
  BOOST_CHECK(tpglibs::testapp::DummyFrameAdapter::validate_frame_view(frame_view));
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestSetAdcGetAdc)
{
  // Create a frame manually (constructor initializes data and adc_words)
  auto frame = std::make_unique<tpglibs::testapp::DUMMY_FRAME_STRUCT>();
  
  // Set values (data is stored as packed uint64_t words)
  frame->set_adc(0, 0, 100);
  frame->set_adc(1, 0, 200);
  frame->set_adc(2, 1, 310);
  frame->set_adc(3, 2, 420);
  
  // Get values and verify (unpacks from uint64_t words)
  BOOST_CHECK_EQUAL(frame->get_adc(0, 0), 100);
  BOOST_CHECK_EQUAL(frame->get_adc(1, 0), 200);
  BOOST_CHECK_EQUAL(frame->get_adc(2, 1), 310);
  BOOST_CHECK_EQUAL(frame->get_adc(3, 2), 420);
  
  // Verify data is stored as packed words (256 * 16 = 4096 words)
  BOOST_CHECK_EQUAL(frame->data.size(), 256 * 16);
}

BOOST_AUTO_TEST_CASE(TestTPGeneratorCompatibility)
{
  // Test that DUMMY_FRAME_STRUCT is compatible with TPGenerator interface
  auto frame = std::make_unique<tpglibs::testapp::DUMMY_FRAME_STRUCT>();
  
  // Verify TPGenerator compatibility members
  BOOST_CHECK_EQUAL(tpglibs::testapp::DUMMY_FRAME_STRUCT::s_num_channels, 64);
  BOOST_CHECK_EQUAL(tpglibs::testapp::DUMMY_FRAME_STRUCT::s_time_samples_per_frame, 256);
  BOOST_CHECK_EQUAL(tpglibs::testapp::DUMMY_FRAME_STRUCT::s_bits_per_adc, 16);
  
  // Verify adc_words pointer is valid
  BOOST_CHECK(frame->adc_words != nullptr);
  
  // Verify get_timestamp method exists
  frame->timestamp = 1234567890;
  BOOST_CHECK_EQUAL(frame->get_timestamp(), 1234567890);
  
  // Verify data storage is correct size (256 * 16 = 4096 words)
  BOOST_CHECK_EQUAL(frame->data.size(), 4096);
}

BOOST_AUTO_TEST_SUITE_END()

