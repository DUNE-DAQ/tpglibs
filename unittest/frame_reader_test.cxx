/**
 * @file frame_reader_test.cxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE frame_reader_test
#define FMT_HEADER_ONLY

#include "tpglibs/testapp/frame/FrameReader.hpp"
#include "test_helpers.hpp"

#include <boost/test/unit_test.hpp>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>

using tpglibs::unittest::write_file_header;
using tpglibs::unittest::write_frame;

BOOST_AUTO_TEST_SUITE(FrameReaderTest)

BOOST_AUTO_TEST_CASE(TestReadSingleFrame)
{
  // Create test file with one frame (fixed 64×256 = 32,768 bytes)
  std::string temp_filename = "/tmp/test_frame_single.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  uint64_t timestamp = 1234567890;
  // Fixed frame size: 64 channels × 256 time samples × 2 bytes = 32,768 bytes
  constexpr size_t frame_data_size = 64 * 256 * sizeof(int16_t);
  std::vector<uint8_t> frame_data(frame_data_size, 0x42);
  write_frame(temp_file, timestamp, frame_data);
  
  temp_file.close();
  
  // Test reading the frame (no size parameter needed - fixed size)
  tpglibs::testapp::FrameReader reader(temp_filename);
  
  auto [status, frame] = reader.next_frame();
  
  BOOST_CHECK(status == tpglibs::testapp::FrameReadStatus::kOk);
  BOOST_CHECK(frame.is_valid());
  BOOST_CHECK_EQUAL(frame.bytes.size(), 8 + frame_data_size);  // header + data
  
  // Verify header content
  uint64_t read_timestamp;
  std::memcpy(&read_timestamp, frame.header(), sizeof(read_timestamp));
  BOOST_CHECK_EQUAL(read_timestamp, timestamp);
  
  // Verify data content
  BOOST_CHECK_EQUAL(frame.data_size(), frame_data_size);
  for (size_t i = 0; i < frame_data_size; ++i) {
    BOOST_CHECK_EQUAL(frame.data()[i], 0x42);
  }
  
  // Should be at EOF now
  auto [status2, frame2] = reader.next_frame();
  BOOST_CHECK(status2 == tpglibs::testapp::FrameReadStatus::kEOF);
  BOOST_CHECK(!frame2.is_valid());
  BOOST_CHECK(reader.eof());
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestReadMultipleFrames)
{
  // Create test file with multiple frames (fixed 64×256)
  std::string temp_filename = "/tmp/test_frame_multiple.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  // Write 3 frames
  constexpr size_t frame_data_size = 64 * 256 * sizeof(int16_t);
  for (int i = 0; i < 3; ++i) {
    uint64_t timestamp = 1000 + i;
    std::vector<uint8_t> frame_data(frame_data_size, static_cast<uint8_t>(i));  // Different data per frame
    write_frame(temp_file, timestamp, frame_data);
  }
  
  temp_file.close();
  
  // Test reading frames sequentially (no size parameter needed)
  tpglibs::testapp::FrameReader reader(temp_filename);
  
  for (int i = 0; i < 3; ++i) {
    auto [status, frame] = reader.next_frame();
    BOOST_CHECK(status == tpglibs::testapp::FrameReadStatus::kOk);
    BOOST_CHECK(frame.is_valid());
    
    uint64_t read_timestamp;
    std::memcpy(&read_timestamp, frame.header(), sizeof(read_timestamp));
    BOOST_CHECK_EQUAL(read_timestamp, 1000 + i);
    
    // Verify data is all the same value (i)
    for (size_t j = 0; j < frame_data_size; ++j) {
      BOOST_CHECK_EQUAL(frame.data()[j], static_cast<uint8_t>(i));
    }
  }
  
  // Next read should be EOF
  auto [status, frame] = reader.next_frame();
  BOOST_CHECK(status == tpglibs::testapp::FrameReadStatus::kEOF);
  BOOST_CHECK(reader.eof());
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestEOF)
{
  // Create test file with only header, no frames
  std::string temp_filename = "/tmp/test_frame_eof.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  // No frames written
  
  temp_file.close();
  
  tpglibs::testapp::FrameReader reader(temp_filename);
  
  // Should immediately return EOF
  auto [status, frame] = reader.next_frame();
  BOOST_CHECK(status == tpglibs::testapp::FrameReadStatus::kEOF);
  BOOST_CHECK(!frame.is_valid());
  BOOST_CHECK(reader.eof());
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestErrorIncompleteFrame)
{
  // Create test file with incomplete frame (missing data)
  std::string temp_filename = "/tmp/test_frame_incomplete.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  // Write frame header but only partial data
  uint64_t timestamp = 1234;
  temp_file.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
  // Write only partial data instead of expected 32,768 bytes
  constexpr size_t expected_data_size = 64 * 256 * sizeof(int16_t);
  std::vector<uint8_t> partial_data(expected_data_size / 2, 0xAA);  // Half the expected size
  temp_file.write(reinterpret_cast<const char*>(partial_data.data()), partial_data.size());
  
  temp_file.close();
  
  tpglibs::testapp::FrameReader reader(temp_filename);
  
  // Should return ERROR for incomplete frame
  auto [status, frame] = reader.next_frame();
  BOOST_CHECK(status == tpglibs::testapp::FrameReadStatus::kError);
  BOOST_CHECK(!frame.is_valid());
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestErrorPartialFrameHeader)
{
  // Create test file with partial frame header (corrupted/incomplete)
  std::string temp_filename = "/tmp/test_frame_partial_header.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  // Write only partial frame header (4 bytes instead of 8)
  uint32_t partial_timestamp = 1234;
  temp_file.write(reinterpret_cast<const char*>(&partial_timestamp), sizeof(partial_timestamp));
  // Missing the remaining 4 bytes of the timestamp - file ends here
  
  temp_file.close();
  
  tpglibs::testapp::FrameReader reader(temp_filename);
  
  // Should return ERROR for partial frame header, not END_OF_FILE
  // A partial frame header is corruption, not a clean EOF
  auto [status, frame] = reader.next_frame();
  BOOST_CHECK(status == tpglibs::testapp::FrameReadStatus::kError);
  BOOST_CHECK(!frame.is_valid());
  BOOST_CHECK(reader.eof());  // Should mark EOF since we can't continue
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestErrorInvalidHeader)
{
  // Create test file with invalid magic number
  std::string temp_filename = "/tmp/test_frame_invalid_magic.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  uint32_t bad_magic = 0xDEADBEEF;  // Invalid magic
  uint32_t version = 0x010004;
  uint32_t reserved = 0x00000000;
  temp_file.write(reinterpret_cast<const char*>(&bad_magic), sizeof(bad_magic));
  temp_file.write(reinterpret_cast<const char*>(&version), sizeof(version));
  temp_file.write(reinterpret_cast<const char*>(&reserved), sizeof(reserved));
  
  temp_file.close();
  
  // Constructor should throw due to invalid header
  BOOST_CHECK_THROW(
    tpglibs::testapp::FrameReader reader(temp_filename),
    std::runtime_error
  );
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestErrorInvalidVersion)
{
  // Create test file with invalid version
  std::string temp_filename = "/tmp/test_frame_invalid_version.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  uint32_t magic = 0x54504754;  // Valid magic
  uint32_t bad_version = 0x99999999;  // Invalid version
  uint32_t reserved = 0x00000000;
  temp_file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
  temp_file.write(reinterpret_cast<const char*>(&bad_version), sizeof(bad_version));
  temp_file.write(reinterpret_cast<const char*>(&reserved), sizeof(reserved));
  
  temp_file.close();
  
  // Constructor should throw due to invalid version
  BOOST_CHECK_THROW(
    tpglibs::testapp::FrameReader reader(temp_filename),
    std::runtime_error
  );
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestReset)
{
  // Create test file with two frames (fixed 64×256)
  std::string temp_filename = "/tmp/test_frame_reset.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  // Write 2 frames
  constexpr size_t frame_data_size = 64 * 256 * sizeof(int16_t);
  for (int i = 0; i < 2; ++i) {
    uint64_t timestamp = 100 + i;
    std::vector<uint8_t> frame_data(frame_data_size, static_cast<uint8_t>(i));
    write_frame(temp_file, timestamp, frame_data);
  }
  
  temp_file.close();
  
  tpglibs::testapp::FrameReader reader(temp_filename);
  
  // Read first frame
  auto [status1, frame1] = reader.next_frame();
  BOOST_CHECK(status1 == tpglibs::testapp::FrameReadStatus::kOk);
  
  uint64_t read_timestamp1;
  std::memcpy(&read_timestamp1, frame1.header(), sizeof(read_timestamp1));
  BOOST_CHECK_EQUAL(read_timestamp1, 100);
  
  // Reset and read again
  reader.reset();
  
  auto [status2, frame2] = reader.next_frame();
  BOOST_CHECK(status2 == tpglibs::testapp::FrameReadStatus::kOk);
  
  uint64_t read_timestamp2;
  std::memcpy(&read_timestamp2, frame2.header(), sizeof(read_timestamp2));
  BOOST_CHECK_EQUAL(read_timestamp2, 100);  // Should read first frame again
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestInvalidFile)
{
  // Try to open non-existent file
  BOOST_CHECK_THROW(
    tpglibs::testapp::FrameReader reader("/tmp/non_existent_frame_file.bin"),
    std::runtime_error
  );
}

BOOST_AUTO_TEST_SUITE_END()

