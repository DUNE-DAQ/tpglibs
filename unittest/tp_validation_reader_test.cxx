/**
 * @file tp_validation_reader_test.cxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE tp_validation_reader_test

#include "tpglibs/testapp/tp/TPValidationReader.hpp"
#include "include/test_helpers.hpp"

#include <boost/test/unit_test.hpp>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>

using tpglibs::unittest::write_file_header;
using tpglibs::unittest::write_tp_record;
using tpglibs::unittest::create_test_tp;

BOOST_AUTO_TEST_SUITE(TPValidationReaderTest)

BOOST_AUTO_TEST_CASE(TestReadSingleFrame)
{
  // Create test file with one frame
  std::string temp_filename = "/tmp/test_tp_reader_single.val";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  // Write one frame with 2 TPs
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps;
  tps.push_back(create_test_tp(1000, 5, 200, 3));
  tps.push_back(create_test_tp(2000, 7, 250, 4));
  
  write_tp_record(temp_file, 0, tps);
  temp_file.close();
  
  // Read using TPValidationReader
  tpglibs::testapp::TPValidationReader reader(temp_filename);
  
  // Check frame exists
  BOOST_CHECK(reader.has_frame(0));
  BOOST_CHECK_EQUAL(reader.num_frames(), 1);
  
  // Get TPs for frame 0
  auto read_tps = reader.get_tps_for_frame(0);
  BOOST_CHECK_EQUAL(read_tps.size(), 2);
  
  // Verify TP content
  BOOST_CHECK_EQUAL(read_tps[0].time_start, 1000);
  BOOST_CHECK_EQUAL(read_tps[0].channel, 5);
  BOOST_CHECK_EQUAL(read_tps[0].adc_peak, 200);
  
  BOOST_CHECK_EQUAL(read_tps[1].time_start, 2000);
  BOOST_CHECK_EQUAL(read_tps[1].channel, 7);
  BOOST_CHECK_EQUAL(read_tps[1].adc_peak, 250);
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestReadMultipleFrames)
{
  // Create test file with multiple frames
  std::string temp_filename = "/tmp/test_tp_reader_multiple.val";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  // Write frame 0 with 2 TPs
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps0;
  tps0.push_back(create_test_tp(1000, 0, 100, 2));
  tps0.push_back(create_test_tp(2000, 1, 200, 3));
  write_tp_record(temp_file, 0, tps0);
  
  // Write frame 5 with 1 TP
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps5;
  tps5.push_back(create_test_tp(5000, 10, 300, 4));
  write_tp_record(temp_file, 5, tps5);
  
  // Write frame 10 with 3 TPs
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps10;
  tps10.push_back(create_test_tp(10000, 20, 400, 5));
  tps10.push_back(create_test_tp(11000, 21, 500, 6));
  tps10.push_back(create_test_tp(12000, 22, 600, 7));
  write_tp_record(temp_file, 10, tps10);
  
  temp_file.close();
  
  // Read using TPValidationReader
  tpglibs::testapp::TPValidationReader reader(temp_filename);
  
  BOOST_CHECK_EQUAL(reader.num_frames(), 3);
  BOOST_CHECK(reader.has_frame(0));
  BOOST_CHECK(reader.has_frame(5));
  BOOST_CHECK(reader.has_frame(10));
  BOOST_CHECK(!reader.has_frame(1));
  BOOST_CHECK(!reader.has_frame(99));
  
  // Get validation frames
  auto validation_frames = reader.get_validation_frames();
  BOOST_CHECK_EQUAL(validation_frames.size(), 3);
  std::sort(validation_frames.begin(), validation_frames.end());
  BOOST_CHECK_EQUAL(validation_frames[0], 0);
  BOOST_CHECK_EQUAL(validation_frames[1], 5);
  BOOST_CHECK_EQUAL(validation_frames[2], 10);
  
  // Verify frame 0
  auto read_tps0 = reader.get_tps_for_frame(0);
  BOOST_CHECK_EQUAL(read_tps0.size(), 2);
  BOOST_CHECK_EQUAL(read_tps0[0].time_start, 1000);
  BOOST_CHECK_EQUAL(read_tps0[1].time_start, 2000);
  
  // Verify frame 5
  auto read_tps5 = reader.get_tps_for_frame(5);
  BOOST_CHECK_EQUAL(read_tps5.size(), 1);
  BOOST_CHECK_EQUAL(read_tps5[0].time_start, 5000);
  BOOST_CHECK_EQUAL(read_tps5[0].channel, 10);
  
  // Verify frame 10
  auto read_tps10 = reader.get_tps_for_frame(10);
  BOOST_CHECK_EQUAL(read_tps10.size(), 3);
  BOOST_CHECK_EQUAL(read_tps10[0].time_start, 10000);
  BOOST_CHECK_EQUAL(read_tps10[2].time_start, 12000);
  
  // Verify non-existent frame returns empty
  auto read_tps_nonexistent = reader.get_tps_for_frame(99);
  BOOST_CHECK_EQUAL(read_tps_nonexistent.size(), 0);
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestEmptyFrame)
{
  // Create test file with one frame that has 0 TPs
  std::string temp_filename = "/tmp/test_tp_reader_empty.val";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  // Write frame with 0 TPs
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> empty_tps;
  write_tp_record(temp_file, 0, empty_tps);
  
  temp_file.close();
  
  // Read using TPValidationReader
  tpglibs::testapp::TPValidationReader reader(temp_filename);
  
  BOOST_CHECK(reader.has_frame(0));
  BOOST_CHECK_EQUAL(reader.num_frames(), 1);
  
  auto read_tps = reader.get_tps_for_frame(0);
  BOOST_CHECK_EQUAL(read_tps.size(), 0);
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestInvalidFileHeader)
{
  // Create test file with invalid magic number
  std::string temp_filename = "/tmp/test_tp_reader_invalid_magic.val";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  uint32_t bad_magic = 0xDEADBEEF;
  uint32_t version = 0x010004;
  uint32_t reserved = 0x00000000;
  temp_file.write(reinterpret_cast<const char*>(&bad_magic), sizeof(bad_magic));
  temp_file.write(reinterpret_cast<const char*>(&version), sizeof(version));
  temp_file.write(reinterpret_cast<const char*>(&reserved), sizeof(reserved));
  
  temp_file.close();
  
  // Constructor should throw due to invalid header
  BOOST_CHECK_THROW(
    tpglibs::testapp::TPValidationReader reader(temp_filename),
    std::runtime_error
  );
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestInvalidVersion)
{
  // Create test file with invalid version
  std::string temp_filename = "/tmp/test_tp_reader_invalid_version.val";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  uint32_t magic = 0x54504754;
  uint32_t bad_version = 0x99999999;
  uint32_t reserved = 0x00000000;
  temp_file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
  temp_file.write(reinterpret_cast<const char*>(&bad_version), sizeof(bad_version));
  temp_file.write(reinterpret_cast<const char*>(&reserved), sizeof(reserved));
  
  temp_file.close();
  
  // Constructor should throw due to invalid version
  BOOST_CHECK_THROW(
    tpglibs::testapp::TPValidationReader reader(temp_filename),
    std::runtime_error
  );
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestInvalidFile)
{
  // Try to open non-existent file
  BOOST_CHECK_THROW(
    tpglibs::testapp::TPValidationReader reader("/tmp/non_existent_tp_file.val"),
    std::runtime_error
  );
}

BOOST_AUTO_TEST_CASE(TestDuplicateFrameIndex)
{
  // Create test file with duplicate frame indices (last write should win)
  std::string temp_filename = "/tmp/test_tp_reader_duplicate.val";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  // Write frame 0 first time
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps0_first;
  tps0_first.push_back(create_test_tp(1000, 0, 100, 2));
  write_tp_record(temp_file, 0, tps0_first);
  
  // Write frame 0 second time (should overwrite)
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps0_second;
  tps0_second.push_back(create_test_tp(2000, 1, 200, 3));
  tps0_second.push_back(create_test_tp(3000, 2, 300, 4));
  write_tp_record(temp_file, 0, tps0_second);
  
  temp_file.close();
  
  // Read using TPValidationReader
  tpglibs::testapp::TPValidationReader reader(temp_filename);
  
  // Should only have one frame (last write wins)
  BOOST_CHECK_EQUAL(reader.num_frames(), 1);
  
  // Should have the second set of TPs
  auto read_tps = reader.get_tps_for_frame(0);
  BOOST_CHECK_EQUAL(read_tps.size(), 2);
  BOOST_CHECK_EQUAL(read_tps[0].time_start, 2000);
  BOOST_CHECK_EQUAL(read_tps[1].time_start, 3000);
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestLargeNumberOfTPs)
{
  // Create test file with many TPs in one frame
  std::string temp_filename = "/tmp/test_tp_reader_large.val";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  
  write_file_header(temp_file);
  
  // Write frame with 100 TPs
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps;
  for (int i = 0; i < 100; ++i) {
    tps.push_back(create_test_tp(1000 + i * 10, i, 100 + i, 2 + i % 5));
  }
  
  write_tp_record(temp_file, 0, tps);
  temp_file.close();
  
  // Read using TPValidationReader
  tpglibs::testapp::TPValidationReader reader(temp_filename);
  
  auto read_tps = reader.get_tps_for_frame(0);
  BOOST_CHECK_EQUAL(read_tps.size(), 100);
  
  // Verify first and last TP
  BOOST_CHECK_EQUAL(read_tps[0].time_start, 1000);
  BOOST_CHECK_EQUAL(read_tps[0].channel, 0);
  BOOST_CHECK_EQUAL(read_tps[99].time_start, 1000 + 99 * 10);
  BOOST_CHECK_EQUAL(read_tps[99].channel, 99);
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_SUITE_END()

