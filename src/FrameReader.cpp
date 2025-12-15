/**
 * @file FrameReader.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/testapp/frame/FrameReader.hpp"
#include <cstring>
#include <stdexcept>
#include <string>

namespace tpglibs {
namespace testapp {

FrameReader::FrameReader(const std::string& filepath)
    : m_reader(filepath),
      m_total_frame_size(FRAME_HEADER_SIZE + FRAME_DATA_SIZE),
      m_eof_reached(false) {
  // Validate file header
  if (!validate_file_header()) {
    throw std::runtime_error("Invalid file header in: " + filepath);
  }
}

bool FrameReader::validate_file_header() {
  // Read file header
  auto header_bytes = m_reader.next(FILE_HEADER_SIZE);
  if (header_bytes.size() != FILE_HEADER_SIZE) {
    return false;
  }
  
  BinaryFileHeader header;
  std::string error;
  // Note: error string is not used here since we throw a generic exception
  // This is acceptable for FrameReader's use case
  return BinaryFileValidator::validate_buffer(
      header_bytes.data(), header_bytes.size(), header, error);
}

std::pair<FrameReadStatus, RawFrameView> FrameReader::next_frame() {
  if (m_eof_reached) {
    return {FrameReadStatus::kEOF, RawFrameView{}};
  }
  
  RawFrameView frame;
  frame.bytes.reserve(m_total_frame_size);
  
  // Read frame header (16 bytes)
  auto header_bytes = m_reader.next(FRAME_HEADER_SIZE);
  if (header_bytes.size() != FRAME_HEADER_SIZE) {
    // Only treat 0 bytes read + EOF as clean kEOF
    // Any partial read (0 < size < FRAME_HEADER_SIZE) is a corrupted/incomplete header = kError
    if (header_bytes.size() == 0 && m_reader.eof()) {
      m_eof_reached = true;
      return {FrameReadStatus::kEOF, RawFrameView{}};
    }
    // Partial frame header is always an error, even if EOF
    // This is never expected to happen, so we throw an error
    m_eof_reached = true;  // Mark EOF since we can't continue
    return {FrameReadStatus::kError, RawFrameView{}};
  }
  
  frame.bytes.insert(frame.bytes.end(), header_bytes.begin(), header_bytes.end());
  
  // Read frame data
  auto data_bytes = m_reader.next(FRAME_DATA_SIZE);
  if (data_bytes.size() != FRAME_DATA_SIZE) {
    // Incomplete frame data is always an error
    // Check EOF to set state consistently
    if (m_reader.eof()) {
      m_eof_reached = true;
    }
    return {FrameReadStatus::kError, RawFrameView{}};  // Incomplete frame is an error
  }
  
  frame.bytes.insert(frame.bytes.end(), data_bytes.begin(), data_bytes.end());
  
  return {FrameReadStatus::kOk, frame};
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

