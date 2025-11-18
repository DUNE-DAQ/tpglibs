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
  // Create test file with one frame
  std::string temp_filename = "/tmp/test_dummy_adapter.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  uint64_t timestamp = 1234567890;
  uint64_t another_key = 9876543210;
  
  // Create test data: 4 channels, 3 time samples
  // Row-major order: [ch0_t0, ch1_t0, ch2_t0, ch3_t0, ch0_t1, ch1_t1, ch2_t1, ch3_t1, ch0_t2, ch1_t2, ch2_t2, ch3_t2]
  std::vector<int16_t> frame_data = {
    100, 200, 300, 400,  // Time 0: channels 0-3
    110, 210, 310, 410,  // Time 1: channels 0-3
    120, 220, 320, 420   // Time 2: channels 0-3
  };
  
  write_frame(temp_file, timestamp, another_key, frame_data);
  temp_file.close();
  
  // Read frame using FrameReader
  const size_t num_channels = 4;
  const size_t num_time_samples = 3;
  const size_t frame_data_size = num_channels * num_time_samples * sizeof(int16_t);
  
  tpglibs::testapp::FrameReader reader(temp_filename, frame_data_size);
  auto [status, frame_view] = reader.next_frame();
  
  BOOST_CHECK(status == tpglibs::testapp::FrameReadStatus::OK);
  BOOST_CHECK(frame_view.is_valid());
  
  // Convert to DUMMY_FRAME_STRUCT using adapter
  auto frame = tpglibs::testapp::DummyFrameAdapter::create_frame(
      frame_view, num_channels, num_time_samples);
  
  BOOST_CHECK(frame != nullptr);
  BOOST_CHECK_EQUAL(frame->timestamp, timestamp);
  BOOST_CHECK_EQUAL(frame->another_key, another_key);
  BOOST_CHECK_EQUAL(frame->num_channels, num_channels);
  BOOST_CHECK_EQUAL(frame->num_time_samples, num_time_samples);
  BOOST_CHECK_EQUAL(frame->data.size(), num_channels * num_time_samples);
  
  // Verify data content (row-major order)
  BOOST_CHECK_EQUAL(frame->get_adc(0, 0), 100);
  BOOST_CHECK_EQUAL(frame->get_adc(1, 0), 200);
  BOOST_CHECK_EQUAL(frame->get_adc(2, 0), 300);
  BOOST_CHECK_EQUAL(frame->get_adc(3, 0), 400);
  
  BOOST_CHECK_EQUAL(frame->get_adc(0, 1), 110);
  BOOST_CHECK_EQUAL(frame->get_adc(1, 1), 210);
  BOOST_CHECK_EQUAL(frame->get_adc(2, 1), 310);
  BOOST_CHECK_EQUAL(frame->get_adc(3, 1), 410);
  
  BOOST_CHECK_EQUAL(frame->get_adc(0, 2), 120);
  BOOST_CHECK_EQUAL(frame->get_adc(1, 2), 220);
  BOOST_CHECK_EQUAL(frame->get_adc(2, 2), 320);
  BOOST_CHECK_EQUAL(frame->get_adc(3, 2), 420);
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestCreateFrameInvalidDimensions)
{
  // Create test file with one frame
  std::string temp_filename = "/tmp/test_dummy_adapter_invalid.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  uint64_t timestamp = 1234;
  uint64_t another_key = 5678;
  std::vector<int16_t> frame_data(12, 0x42);  // 4 channels * 3 time samples
  
  write_frame(temp_file, timestamp, another_key, frame_data);
  temp_file.close();
  
  // Read frame
  tpglibs::testapp::FrameReader reader(temp_filename, 12 * sizeof(int16_t));
  auto [status, frame_view] = reader.next_frame();
  BOOST_CHECK(status == tpglibs::testapp::FrameReadStatus::OK);
  
  // Try to create frame with invalid dimensions (0 channels)
  auto frame1 = tpglibs::testapp::DummyFrameAdapter::create_frame(
      frame_view, 0, 3);
  BOOST_CHECK(frame1 == nullptr);
  
  // Try to create frame with invalid dimensions (0 time samples)
  auto frame2 = tpglibs::testapp::DummyFrameAdapter::create_frame(
      frame_view, 4, 0);
  BOOST_CHECK(frame2 == nullptr);
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestCreateFrameSizeMismatch)
{
  // Create test file with one frame
  std::string temp_filename = "/tmp/test_dummy_adapter_mismatch.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  uint64_t timestamp = 1234;
  uint64_t another_key = 5678;
  std::vector<int16_t> frame_data(12, 0x42);  // 4 channels * 3 time samples = 12 values
  
  write_frame(temp_file, timestamp, another_key, frame_data);
  temp_file.close();
  
  // Read frame
  tpglibs::testapp::FrameReader reader(temp_filename, 12 * sizeof(int16_t));
  auto [status, frame_view] = reader.next_frame();
  BOOST_CHECK(status == tpglibs::testapp::FrameReadStatus::OK);
  
  // Try to create frame with wrong dimensions (should fail size validation)
  auto frame1 = tpglibs::testapp::DummyFrameAdapter::create_frame(
      frame_view, 4, 4);  // Expects 4*4=16 values, but frame has 12
  BOOST_CHECK(frame1 == nullptr);
  
  auto frame2 = tpglibs::testapp::DummyFrameAdapter::create_frame(
      frame_view, 6, 2);  // Expects 6*2=12 values, matches!
  BOOST_CHECK(frame2 != nullptr);
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestCreateFrameInvalidView)
{
  // Create invalid frame view (empty)
  tpglibs::testapp::RawFrameView invalid_view;
  invalid_view.bytes.clear();  // Empty view
  
  auto frame = tpglibs::testapp::DummyFrameAdapter::create_frame(
      invalid_view, 4, 3);
  BOOST_CHECK(frame == nullptr);
}

BOOST_AUTO_TEST_CASE(TestCalculateFrameSize)
{
  // Test frame size calculation
  size_t size1 = tpglibs::testapp::DummyFrameAdapter::calculate_frame_size(4, 3);
  // Header: 16 bytes, Data: 4*3*2 = 24 bytes, Total: 40 bytes
  BOOST_CHECK_EQUAL(size1, 16 + 24);
  
  size_t size2 = tpglibs::testapp::DummyFrameAdapter::calculate_frame_size(64, 256);
  // Header: 16 bytes, Data: 64*256*2 = 32768 bytes, Total: 32784 bytes
  BOOST_CHECK_EQUAL(size2, 16 + 32768);
  
  size_t size3 = tpglibs::testapp::DummyFrameAdapter::calculate_frame_data_size(4, 3);
  BOOST_CHECK_EQUAL(size3, 24);
  
  size_t size4 = tpglibs::testapp::DummyFrameAdapter::calculate_frame_data_size(64, 256);
  BOOST_CHECK_EQUAL(size4, 32768);
}

BOOST_AUTO_TEST_CASE(TestValidateDimensions)
{
  // Valid dimensions
  BOOST_CHECK(tpglibs::testapp::DummyFrameAdapter::validate_dimensions(4, 3));
  BOOST_CHECK(tpglibs::testapp::DummyFrameAdapter::validate_dimensions(64, 256));
  BOOST_CHECK(tpglibs::testapp::DummyFrameAdapter::validate_dimensions(1, 1));
  
  // Invalid dimensions
  BOOST_CHECK(!tpglibs::testapp::DummyFrameAdapter::validate_dimensions(0, 3));
  BOOST_CHECK(!tpglibs::testapp::DummyFrameAdapter::validate_dimensions(4, 0));
  BOOST_CHECK(!tpglibs::testapp::DummyFrameAdapter::validate_dimensions(0, 0));
}

BOOST_AUTO_TEST_CASE(TestValidateFrameView)
{
  // Create test file with one frame
  std::string temp_filename = "/tmp/test_dummy_adapter_validate.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  uint64_t timestamp = 1234;
  uint64_t another_key = 5678;
  std::vector<int16_t> frame_data(12, 0x42);  // 4 channels * 3 time samples
  
  write_frame(temp_file, timestamp, another_key, frame_data);
  temp_file.close();
  
  // Read frame
  tpglibs::testapp::FrameReader reader(temp_filename, 12 * sizeof(int16_t));
  auto [status, frame_view] = reader.next_frame();
  BOOST_CHECK(status == tpglibs::testapp::FrameReadStatus::OK);
  
  // Validate with correct dimensions
  BOOST_CHECK(tpglibs::testapp::DummyFrameAdapter::validate_frame_view(
      frame_view, 4, 3));
  
  // Validate with wrong dimensions (different total size)
  BOOST_CHECK(!tpglibs::testapp::DummyFrameAdapter::validate_frame_view(
      frame_view, 4, 4));  // 4*4*2 = 32 bytes data, total 48 bytes (wrong)
  BOOST_CHECK(!tpglibs::testapp::DummyFrameAdapter::validate_frame_view(
      frame_view, 8, 2));  // 8*2*2 = 32 bytes data, total 48 bytes (wrong)
  // Note: 6*2*2 = 24 bytes data, total 40 bytes - same as 4*3, so would pass
  // But 4*3 is the correct interpretation based on the test data layout
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestSetAdcGetAdc)
{
  // Create a frame manually
  auto frame = std::make_unique<tpglibs::testapp::DUMMY_FRAME_STRUCT>();
  frame->num_channels = 4;
  frame->num_time_samples = 3;
  frame->data.resize(12, 0);
  
  // Set values
  frame->set_adc(0, 0, 100);
  frame->set_adc(1, 0, 200);
  frame->set_adc(2, 1, 310);
  frame->set_adc(3, 2, 420);
  
  // Get values and verify
  BOOST_CHECK_EQUAL(frame->get_adc(0, 0), 100);
  BOOST_CHECK_EQUAL(frame->get_adc(1, 0), 200);
  BOOST_CHECK_EQUAL(frame->get_adc(2, 1), 310);
  BOOST_CHECK_EQUAL(frame->get_adc(3, 2), 420);
  
  // Verify row-major layout
  BOOST_CHECK_EQUAL(frame->data[0 * 4 + 0], 100);  // time 0, channel 0
  BOOST_CHECK_EQUAL(frame->data[0 * 4 + 1], 200);  // time 0, channel 1
  BOOST_CHECK_EQUAL(frame->data[1 * 4 + 2], 310);  // time 1, channel 2
  BOOST_CHECK_EQUAL(frame->data[2 * 4 + 3], 420);  // time 2, channel 3
}

BOOST_AUTO_TEST_CASE(TestLargeFrame)
{
  // Test with larger frame (64 channels, 256 time samples - typical TPGenerator config)
  std::string temp_filename = "/tmp/test_dummy_adapter_large.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  uint64_t timestamp = 9999999999;
  uint64_t another_key = 8888888888;
  
  const size_t num_channels = 64;
  const size_t num_time_samples = 256;
  std::vector<int16_t> frame_data(num_channels * num_time_samples);
  
  // Fill with test pattern: value = (time_sample * 100 + channel) % 1000
  for (size_t t = 0; t < num_time_samples; ++t) {
    for (size_t c = 0; c < num_channels; ++c) {
      frame_data[t * num_channels + c] = static_cast<int16_t>((t * 100 + c) % 1000);
    }
  }
  
  write_frame(temp_file, timestamp, another_key, frame_data);
  temp_file.close();
  
  // Read and convert
  const size_t frame_data_size = num_channels * num_time_samples * sizeof(int16_t);
  tpglibs::testapp::FrameReader reader(temp_filename, frame_data_size);
  auto [status, frame_view] = reader.next_frame();
  
  BOOST_CHECK(status == tpglibs::testapp::FrameReadStatus::OK);
  
  auto frame = tpglibs::testapp::DummyFrameAdapter::create_frame(
      frame_view, num_channels, num_time_samples);
  
  BOOST_CHECK(frame != nullptr);
  BOOST_CHECK_EQUAL(frame->timestamp, timestamp);
  BOOST_CHECK_EQUAL(frame->another_key, another_key);
  BOOST_CHECK_EQUAL(frame->num_channels, num_channels);
  BOOST_CHECK_EQUAL(frame->num_time_samples, num_time_samples);
  
  // Verify a few sample values
  BOOST_CHECK_EQUAL(frame->get_adc(0, 0), 0);
  BOOST_CHECK_EQUAL(frame->get_adc(1, 0), 1);
  BOOST_CHECK_EQUAL(frame->get_adc(0, 1), 100);
  BOOST_CHECK_EQUAL(frame->get_adc(63, 255), static_cast<int16_t>((255 * 100 + 63) % 1000));
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_SUITE_END()

