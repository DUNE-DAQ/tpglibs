/**
 * @file FrameReader.hpp
 *
 * @brief Frame reader for binary frame files - reads frames sequentially
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_TESTAPP_FRAMEREADER_HPP_
#define TPGLIBS_TESTAPP_FRAMEREADER_HPP_

#include "tpglibs/testapp/common/BinaryFileValidator.hpp"
#include "tpglibs/testapp/reader/BinarySignalReader.hpp"
#include <cstdint>
#include <vector>
#include <string>

namespace tpglibs {
namespace testapp {

/**
 * @brief Status codes for frame reading operations
 */
enum class FrameReadStatus {
  OK,           ///< Frame read successfully
  END_OF_FILE,  ///< End of file reached (normal condition)
  ERROR         ///< Fatal error (invalid header, malformed frame, I/O error)
};

/**
 * @brief Raw frame view - contains frame header and data
 *
 * This is a simple container for raw frame bytes that only divides the frame into header and data.
 * The frame consists of:
 * - Header: 16 bytes (8 bytes timestamp + 8 bytes another_key)
 * - Data: frame_data_size bytes
 */
struct RawFrameView {
  std::vector<uint8_t> bytes;  ///< Complete frame: header (16 bytes) + data
  
  /**
   * @brief Get pointer to frame header (first 16 bytes)
   * @warning Returns nullptr if frame is invalid (bytes.size() < 16)
   */
  const uint8_t* header() const { 
    return (bytes.size() >= 16) ? bytes.data() : nullptr; 
  }
  
  /**
   * @brief Get pointer to frame data (bytes after header)
   * @warning Returns nullptr if frame is invalid (bytes.size() < 16)
   */
  const uint8_t* data() const { 
    // simple check to see if content exists after the header
    return (bytes.size() >= 16) ? bytes.data() + 16 : nullptr; 
  }
  
  /**
   * @brief Get frame data size, which is the size of the frame minus the header size
   * @warning Returns 0 if frame is invalid (bytes.size() < 16) to avoid unsigned underflow
   */
  size_t data_size() const { 
    return (bytes.size() >= 16) ? bytes.size() - 16 : 0; 
  }
  
  /**
   * @brief Check if frame is valid (has header and data)
   */
  bool is_valid() const { return bytes.size() >= 16; }
};

/**
 * @brief Frame reader for binary frame files
 *
 * Reads frames sequentially from a binary file. Each frame consists of:
 * - Frame header: 16 bytes (timestamp + another_key)
 * - Frame data: configurable size (typically num_channels * num_time_samples * 2)
 * 2 stands for 2 bytes per sample, which is the size of int16_t
 *
 * The file must start with a BinaryFileHeader (12 bytes, defined for this test app, in common/BinaryFileValidator.hpp) which is validated
 * on construction.
 *
 * Error semantics:
 * - EOF: Normal end of file, no error
 * - ERROR: Fatal error (invalid header, malformed frame, I/O error)
 *
 * Reset semantics:
 * - reset() seeks back to start of frame data (after file header)
 * - Assumes file header is still valid (no revalidation)
 */
class FrameReader {
 public:
  /**
   * @brief Constructor - opens file and validates header
   * @param filepath Path to binary frame file
   * @throws std::runtime_error if file cannot be opened or header is invalid
   * 
   * Uses fixed frame size for 64 channels × 256 time samples
   */
  explicit FrameReader(const std::string& filepath);
  
  /**
   * @brief Destructor
   */
  ~FrameReader() = default;
  
  // Non-copyable
  FrameReader(const FrameReader&) = delete;
  FrameReader& operator=(const FrameReader&) = delete;
  
  // Movable
  FrameReader(FrameReader&&) = default;
  FrameReader& operator=(FrameReader&&) = default;
  
  /**
   * @brief Read next frame from file
   * @return Pair of (status, frame_view)
   *   - status == OK: Frame read successfully, frame_view contains data
   *   - status == END_OF_FILE: End of file reached, frame_view is empty
   *   - status == ERROR: Fatal error occurred, frame_view is empty
   */
  std::pair<FrameReadStatus, RawFrameView> next_frame();
  
  /**
   * @brief Check if end of file reached
   * @return true if EOF reached
   */
  bool eof();
  
  /**
   * @brief Reset reader to start of frame data (after file header)
   *
   * Seeks back to position after the 12-byte file header.
   * Does not revalidate the file header - assumes it's still valid.
   */
  void reset();
  
  /**
   * @brief Get current file position (relative to start of file)
   */
  std::streampos tellg();

 private:
  BinarySignalReader<uint8_t> m_reader;
  size_t m_total_frame_size;  // 16 (header) + frame_data_size
  bool m_eof_reached;
  static constexpr size_t FILE_HEADER_SIZE = BinaryFileValidator::s_header_size;
  static constexpr size_t FRAME_HEADER_SIZE = 16; // Frame header size
  // Fixed frame data size for 64 channels × 256 time samples (unpacked int16_t format)
  static constexpr size_t FRAME_DATA_SIZE = 64 * 256 * sizeof(int16_t);  // 32,768 bytes
  
  /**
   * @brief Validate file header (magic number and version)
   * @return true if valid, false otherwise
   */
  bool validate_file_header();
};

} // namespace testapp
} // namespace tpglibs

#endif // TPGLIBS_TESTAPP_FRAMEREADER_HPP_

