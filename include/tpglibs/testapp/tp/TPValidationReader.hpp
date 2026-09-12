/**
 * @file TPValidationReader.hpp
 *
 * @brief Reader for TP validation binary files
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_TESTAPP_TPVALIDATIONREADER_HPP_
#define TPGLIBS_TESTAPP_TPVALIDATIONREADER_HPP_

#include "tpglibs/testapp/common/BinaryFileValidator.hpp"
#include "trgdataformats/TriggerPrimitive.hpp"
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <climits>

namespace tpglibs {
namespace testapp {

/**
 * @brief Reader for TP validation binary files
 *
 * Reads TP validation files and provides indexed access to TPs by frame index.
 * Builds entire index at construction (fine for small validation files).
 *
 * File format:
 * - File header: 12 bytes (magic, version, reserved)
 * - For each frame: frame_index (4 bytes), num_tps (4 bytes), TP data
 */
class TPValidationReader {
 public:
  /**
   * @brief Constructor - opens file and builds index
   * @param filepath Path to TP validation binary file (.val)
   * @throws std::runtime_error if file cannot be opened or header is invalid
   */
  explicit TPValidationReader(const std::string& filepath);
  
  /**
   * @brief Destructor
   */
  ~TPValidationReader() = default;
  
  // Non-copyable
  TPValidationReader(const TPValidationReader&) = delete;
  TPValidationReader& operator=(const TPValidationReader&) = delete;
  
  // Movable
  TPValidationReader(TPValidationReader&&) = default;
  TPValidationReader& operator=(TPValidationReader&&) = default;
  
  /**
   * @brief Get TPs for a specific frame index
   * @param frame_index Frame index to look up
   * @return Vector of TriggerPrimitive objects, or empty vector if frame_index not found
   */
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> 
  get_tps_for_frame(uint32_t frame_index) const;
  
  /**
   * @brief Get all validation frame indices
   * @return Vector of frame indices that have TPs in the validation file
   */
  std::vector<uint32_t> get_validation_frames() const;
  
  /**
   * @brief Check if frame index exists in validation file
   * @param frame_index Frame index to check
   * @return true if frame_index has TPs in the validation file
   */
  bool has_frame(uint32_t frame_index) const;
  
  /**
   * @brief Get number of frames in validation file
   * @return Number of frame indices with TPs
   */
  size_t num_frames() const;

 private:
  static constexpr size_t s_FILE_HEADER_SIZE = BinaryFileValidator::s_header_size;
  static constexpr size_t s_FRAME_INDEX_SIZE = sizeof(uint32_t);
  static constexpr size_t s_TP_COUNT_SIZE = sizeof(uint32_t);
  
  // Index: frame_index -> TPs
  std::unordered_map<uint32_t, std::vector<dunedaq::trgdataformats::TriggerPrimitive>> m_index;
  
  /**
   * @brief Validate file header (magic number and version)
   * @param file Input file stream (positioned at start)
   * @return true if valid, false otherwise
   */
  bool validate_file_header(std::ifstream& file, std::string& error);
  
  /**
   * @brief Build index by reading all TP records from file
   * @param file Input file stream (positioned after file header)
   */
  void build_index(std::ifstream& file);
};

} // namespace testapp
} // namespace tpglibs

#endif // TPGLIBS_TESTAPP_TPVALIDATIONREADER_HPP_

