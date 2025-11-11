/**
 * @file BinarySignalReader.hxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_TESTAPP_BINARYSIGNALREADER_HXX_
#define TPGLIBS_TESTAPP_BINARYSIGNALREADER_HXX_

#include "BinarySignalReader.hpp"
#include <iostream>

namespace tpglibs {
namespace testapp {

template<typename T>
BinarySignalReader<T>::BinarySignalReader(const std::string& filepath) 
    : m_file(filepath, std::ios::binary), m_eof_reached(false) {
  if (!m_file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filepath);
  }
}

template<typename T>
std::vector<T> BinarySignalReader<T>::next(size_t n) {
  if (m_eof_reached || n == 0) {
    return std::vector<T>();
  }
  
  std::vector<T> result;
  result.reserve(n);
  
  for (size_t i = 0; i < n; ++i) {
    T value;
    m_file.read(reinterpret_cast<char*>(&value), sizeof(T));
    
    if (m_file.gcount() != sizeof(T)) {
      m_eof_reached = true;
      break;
    }
    
    result.push_back(value);
  }
  
  return result;
}

template<typename T>
bool BinarySignalReader<T>::eof() {
  // Check if we're at the end of the file by comparing position with file size
  std::streampos current_pos = m_file.tellg();
  m_file.seekg(0, std::ios::end);
  std::streampos end_pos = m_file.tellg();
  m_file.seekg(current_pos);
  
  if (current_pos >= end_pos) {
    m_eof_reached = true;
    return true;
  }
  
  m_eof_reached = false;
  return false;
}

template<typename T>
std::streampos BinarySignalReader<T>::tellg() {
  return m_file.tellg();
}

template<typename T>
void BinarySignalReader<T>::seekg(std::streampos pos) {
  m_file.seekg(pos);
  eof(); // Update m_eof_reached based on actual position
}

} // namespace testapp
} // namespace tpglibs

#endif // TPGLIBS_TESTAPP_BINARYSIGNALREADER_HXX_
