/**
 * @file FrameReader.hxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_TESTAPP_FRAMEREADER_HXX_
#define TPGLIBS_TESTAPP_FRAMEREADER_HXX_

#include "FrameReader.hpp"
#include <cstring>
#include <stdexcept>

namespace tpglibs {
namespace testapp {

struct BinaryFileHeader {
  uint32_t magic_number;
  uint32_t version;
  uint32_t reserved;
};

FrameReader::FrameReader(const std::string& filepath, size_t frame_data_size)
    : m_reader(filepath),
      m_frame_data_size(frame_data_size),
      m_total_frame_size(FRAME_HEADER_SIZE + frame_data_size),
      m_eof_reached(false) {
  // Validate file header
  if (!validate_file_header()) {
    throw std::runtime_error("Invalid file header in: " + filepath);
  }
}

bool FrameReader::validate_file_header() {
  // Read file header (12 bytes)
  auto header_bytes = m_reader.next(FILE_HEADER_SIZE);
  if (header_bytes.size() != FILE_HEADER_SIZE) {
    return false;
  }
  
  BinaryFileHeader header;
  std::memcpy(&header, header_bytes.data(), sizeof(BinaryFileHeader));
  
  // Validate magic number: 0x54504754 ("TPGT")
  if (header.magic_number != 0x54504754) {
    return false;
  }
  
  // Validate version: 0x010004 (1.0.4)
  if (header.version != 0x010004) {
    return false;
  }
  
  return true;
}

std::pair<FrameReadStatus, RawFrameView> FrameReader::next_frame() {
  if (m_eof_reached) {
    return {FrameReadStatus::END_OF_FILE, RawFrameView{}};
  }
  
  RawFrameView frame;
  frame.bytes.reserve(m_total_frame_size);
  
  // Read frame header (16 bytes)
  auto header_bytes = m_reader.next(FRAME_HEADER_SIZE);
  if (header_bytes.size() != FRAME_HEADER_SIZE) {
    // Only treat 0 bytes read + EOF as clean END_OF_FILE
    // Any partial read (0 < size < FRAME_HEADER_SIZE) is a corrupted/incomplete header = ERROR
    if (header_bytes.size() == 0 && m_reader.eof()) {
      m_eof_reached = true;
      return {FrameReadStatus::END_OF_FILE, RawFrameView{}};
    }
    // Partial frame header is always an error, even if EOF
    m_eof_reached = true;  // Mark EOF since we can't continue
    return {FrameReadStatus::ERROR, RawFrameView{}};
  }
  
  frame.bytes.insert(frame.bytes.end(), header_bytes.begin(), header_bytes.end());
  
  // Read frame data
  auto data_bytes = m_reader.next(m_frame_data_size);
  if (data_bytes.size() != m_frame_data_size) {
    // Incomplete frame data is always an error
    // Check EOF to set state consistently
    if (m_reader.eof()) {
      m_eof_reached = true;
    }
    return {FrameReadStatus::ERROR, RawFrameView{}};  // Incomplete frame is an error
  }
  
  frame.bytes.insert(frame.bytes.end(), data_bytes.begin(), data_bytes.end());
  
  return {FrameReadStatus::OK, frame};
}

bool FrameReader::eof() {
  if (m_eof_reached) {
    return true;
  }
  m_eof_reached = m_reader.eof();
  return m_eof_reached;
}

void FrameReader::reset() {
  // Seek to position after file header
  m_reader.seekg(FILE_HEADER_SIZE);
  m_eof_reached = false;
}

std::streampos FrameReader::tellg() {
  return m_reader.tellg();
}

} // namespace testapp
} // namespace tpglibs

#endif // TPGLIBS_TESTAPP_FRAMEREADER_HXX_

