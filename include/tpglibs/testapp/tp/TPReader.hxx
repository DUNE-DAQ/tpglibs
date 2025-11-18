/**
 * @file TPReader.hxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_TESTAPP_TPREADER_HXX_
#define TPGLIBS_TESTAPP_TPREADER_HXX_

#include "TPReader.hpp"
#include <fstream>
#include <cstring>
#include <stdexcept>
#include <climits>
#include <limits>

namespace tpglibs {
namespace testapp {

struct BinaryFileHeader {
  uint32_t magic_number;
  uint32_t version;
  uint32_t reserved;
};

TPReader::TPReader(const std::string& filepath) {
  std::ifstream file(filepath, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open TP validation file: " + filepath);
  }
  
  // Validate file header
  if (!validate_file_header(file)) {
    throw std::runtime_error("Invalid file header in TP validation file: " + filepath);
  }
  
  // Build index from remaining file content
  build_index(file);
  
  file.close();
}

bool TPReader::validate_file_header(std::ifstream& file) {
  BinaryFileHeader header;
  file.read(reinterpret_cast<char*>(&header), sizeof(BinaryFileHeader));
  
  if (file.gcount() != sizeof(BinaryFileHeader)) {
    return false;
  }
  
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

void TPReader::build_index(std::ifstream& file) {
  const size_t tp_size = sizeof(dunedaq::trgdataformats::TriggerPrimitive);
  
  // Reasonable maximum: 1 million TPs per frame (prevents DoS and overflow)
  constexpr uint32_t MAX_TPS_PER_FRAME = 1000000;
  
  while (file.good()) {
    // Read frame index
    uint32_t frame_index;
    file.read(reinterpret_cast<char*>(&frame_index), FRAME_INDEX_SIZE);
    
    // Check if read was successful
    if (!file.good() || file.gcount() != FRAME_INDEX_SIZE) {
      // End of file (normal) or read error
      if (file.eof() && file.gcount() == 0) {
        // Clean EOF - no more records
        break;
      }
      // Partial read indicates corrupted file - stop building index
      // Note: This is acceptable for validation files - partial data is better than crashing
      break;
    }
    
    // Read TP count
    uint32_t num_tps;
    file.read(reinterpret_cast<char*>(&num_tps), TP_COUNT_SIZE);
    
    if (!file.good() || file.gcount() != TP_COUNT_SIZE) {
      // Incomplete record - stop reading
      break;
    }
    
    // Validate num_tps to prevent DoS and integer overflow
    if (num_tps > MAX_TPS_PER_FRAME) {
      // Skip this record - invalid num_tps (likely corrupted file)
      // Don't throw to allow reading other valid records if any
      break;
    }
    
    // Check for integer overflow in expected_bytes calculation
    // If num_tps * tp_size would overflow size_t, skip this record
    const size_t max_safe_tps = std::numeric_limits<size_t>::max() / tp_size;
    if (num_tps > max_safe_tps) {
      // Would overflow - skip this record
      break;
    }
    
    // Read TPs
    std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps;
    try {
      tps.resize(num_tps);
    } catch (const std::bad_alloc&) {
      // Memory allocation failed - skip this record
      break;
    }
    
    const size_t expected_bytes = num_tps * tp_size;
    file.read(reinterpret_cast<char*>(tps.data()), expected_bytes);
    
    // Check if read was successful
    if (!file.good() || file.gcount() != static_cast<std::streamsize>(expected_bytes)) {
      // Incomplete TP data - skip this record
      break;
    }
    
    // Store in index (overwrites if frame_index already exists - last write wins)
    m_index[frame_index] = std::move(tps);
  }
}

std::vector<dunedaq::trgdataformats::TriggerPrimitive> 
TPReader::get_tps_for_frame(uint32_t frame_index) const {
  auto it = m_index.find(frame_index);
  if (it != m_index.end()) {
    return it->second;  // Return copy of TPs
  }
  return std::vector<dunedaq::trgdataformats::TriggerPrimitive>();  // Empty vector if not found
}

std::vector<uint32_t> TPReader::get_validation_frames() const {
  std::vector<uint32_t> frames;
  frames.reserve(m_index.size());
  for (const auto& pair : m_index) {
    frames.push_back(pair.first);
  }
  return frames;
}

bool TPReader::has_frame(uint32_t frame_index) const {
  return m_index.find(frame_index) != m_index.end();
}

size_t TPReader::num_frames() const {
  return m_index.size();
}

} // namespace testapp
} // namespace tpglibs

#endif // TPGLIBS_TESTAPP_TPREADER_HXX_

