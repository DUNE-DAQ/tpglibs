/**
 * @file binary_signal_reader_test.cxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE binary_signal_reader_test

#include "tpglibs/testapp/reader/BinarySignalReader.hpp"

#include <boost/test/unit_test.hpp>
#include <sstream>
#include <vector>
#include <cstdint>

BOOST_AUTO_TEST_SUITE(BinarySignalReaderTest)

BOOST_AUTO_TEST_CASE(TestInt16Read)
{
  // Create test data: 5 int16 values
  std::vector<int16_t> test_data = {100, -200, 300, -400, 500};
  
  // Write to stringstream as binary data
  std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
  for (const auto& value : test_data) {
    ss.write(reinterpret_cast<const char*>(&value), sizeof(int16_t));
  }
  
  // Reset to beginning
  ss.seekg(0);
  
  // Create reader from stringstream (we need to modify BinarySignalReader to accept istream)
  // For now, let's create a temporary file approach
  std::string temp_filename = "/tmp/test_binary_data.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  temp_file.write(ss.str().c_str(), ss.str().size());
  temp_file.close();
  
  // Test the reader
  tpglibs::testapp::BinarySignalReader<int16_t> reader(temp_filename);
  
  // Read all data
  auto result = reader.next(5);
  
  BOOST_CHECK_EQUAL(result.size(), 5);
  for (size_t i = 0; i < test_data.size(); ++i) {
    BOOST_CHECK_EQUAL(result[i], test_data[i]);
  }
  
  // Should be at EOF
  BOOST_CHECK(reader.eof());
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestPartialRead)
{
  // Create test data: 10 int16 values
  std::vector<int16_t> test_data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  
  std::string temp_filename = "/tmp/test_partial_read.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  for (const auto& value : test_data) {
    temp_file.write(reinterpret_cast<const char*>(&value), sizeof(int16_t));
  }
  temp_file.close();
  
  tpglibs::testapp::BinarySignalReader<int16_t> reader(temp_filename);
  
  // Read first 3 elements
  auto result1 = reader.next(3);
  BOOST_CHECK_EQUAL(result1.size(), 3);
  BOOST_CHECK_EQUAL(result1[0], 1);
  BOOST_CHECK_EQUAL(result1[1], 2);
  BOOST_CHECK_EQUAL(result1[2], 3);
  BOOST_CHECK(!reader.eof());
  
  // Read next 4 elements
  auto result2 = reader.next(4);
  BOOST_CHECK_EQUAL(result2.size(), 4);
  BOOST_CHECK_EQUAL(result2[0], 4);
  BOOST_CHECK_EQUAL(result2[1], 5);
  BOOST_CHECK_EQUAL(result2[2], 6);
  BOOST_CHECK_EQUAL(result2[3], 7);
  BOOST_CHECK(!reader.eof());
  
  // Read remaining elements
  auto result3 = reader.next(5);
  BOOST_CHECK_EQUAL(result3.size(), 3); // Only 3 left
  BOOST_CHECK_EQUAL(result3[0], 8);
  BOOST_CHECK_EQUAL(result3[1], 9);
  BOOST_CHECK_EQUAL(result3[2], 10);
  BOOST_CHECK(reader.eof());
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestEmptyFile)
{
  std::string temp_filename = "/tmp/test_empty.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  temp_file.close();
  
  tpglibs::testapp::BinarySignalReader<int16_t> reader(temp_filename);
  
  auto result = reader.next(5);
  BOOST_CHECK_EQUAL(result.size(), 0);
  BOOST_CHECK(reader.eof());
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestSeekAndTell)
{
  // Create test data: 5 int16 values
  std::vector<int16_t> test_data = {10, 20, 30, 40, 50};
  
  std::string temp_filename = "/tmp/test_seek.bin";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  for (const auto& value : test_data) {
    temp_file.write(reinterpret_cast<const char*>(&value), sizeof(int16_t));
  }
  temp_file.close();
  
  tpglibs::testapp::BinarySignalReader<int16_t> reader(temp_filename);
  
  // Read first element
  auto result1 = reader.next(1);
  BOOST_CHECK_EQUAL(result1.size(), 1);
  BOOST_CHECK_EQUAL(result1[0], 10);
  
  // Check position
  auto pos = reader.tellg();
  BOOST_CHECK_EQUAL(pos, sizeof(int16_t));
  
  // Seek to beginning
  reader.seekg(0);
  
  // Read all elements
  auto result2 = reader.next(5);
  BOOST_CHECK_EQUAL(result2.size(), 5);
  for (size_t i = 0; i < test_data.size(); ++i) {
    BOOST_CHECK_EQUAL(result2[i], test_data[i]);
  }
  
  // Clean up
  std::remove(temp_filename.c_str());
}

BOOST_AUTO_TEST_CASE(TestInvalidFile)
{
  // Try to open non-existent file
  BOOST_CHECK_THROW(tpglibs::testapp::BinarySignalReader<int16_t>("/tmp/non_existent_file.bin"), 
                    std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
