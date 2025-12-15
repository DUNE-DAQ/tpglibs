/**
 * @file BinaryFileValidator.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/testapp/common/BinaryFileValidator.hpp"
#include <cstring>
#include <string>
#include <fstream>

namespace tpglibs {
namespace testapp {

bool BinaryFileValidator::validate_stream(std::istream& stream,
                                          BinaryFileHeader& header,
                                          std::string& error) {
  stream.read(reinterpret_cast<char*>(&header), sizeof(BinaryFileHeader));
  const std::streamsize bytes_read = stream.gcount();
  if (bytes_read != static_cast<std::streamsize>(s_header_size)) {
    error = "Failed to read binary file header (expected " + 
            std::to_string(s_header_size) + " bytes, got " + 
            std::to_string(bytes_read) + ")";
    return false;
  }
  if (stream.bad()) {
    error = "Stream error while reading binary file header";
    return false;
  }
  // EOF after reading exactly s_header_size bytes is acceptable, so we don't check it
  return validate_header_fields(header, error);
}

bool BinaryFileValidator::validate_buffer(const uint8_t* data,
                                          size_t size,
                                          BinaryFileHeader& header,
                                          std::string& error) {
  if (size < s_header_size) {
    error = "Insufficient bytes for binary file header";
    return false;
  }
  std::memcpy(&header, data, sizeof(BinaryFileHeader));
  return validate_header_fields(header, error);
}

bool BinaryFileValidator::validate_file(const std::string& filepath,
                                        BinaryFileHeader& header,
                                        std::string& error) {
  std::ifstream stream(filepath, std::ios::binary);
  if (!stream.is_open()) {
    error = "Failed to open file: " + filepath;
    return false;
  }
  return validate_stream(stream, header, error);
}

bool BinaryFileValidator::validate_header_fields(const BinaryFileHeader& header,
                                                 std::string& error) {
  if (header.magic_number != s_magic_number) {
    error = "Invalid magic number in binary file header";
    return false;
  }
  if (header.version != s_version) {
    error = "Unsupported binary file version";
    return false;
  }
  return true;
}

} // namespace testapp
} // namespace tpglibs

