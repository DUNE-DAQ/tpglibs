/**
 * @file BinaryFileValidator.hpp
 *
 * @brief Utility for validating binary file headers shared across test apps
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_TESTAPP_BINARYFILEVALIDATOR_HPP_
#define TPGLIBS_TESTAPP_BINARYFILEVALIDATOR_HPP_

#include <cstddef>
#include <cstdint>
#include <istream>
#include <string>

namespace tpglibs {
namespace testapp {

/**
 * @brief Binary file header used by frame/TP validation files.
 *
 * Layout is fixed to 12 bytes (magic, version, reserved).
 */
struct BinaryFileHeader {
  uint32_t magic_number;
  uint32_t version;
  uint32_t reserved;
};

/**
 * @brief Validator for binary file headers used by test applications.
 *
 * Provides helpers to validate headers read from streams or in-memory buffers.
 * Uses bool + error string for clear, non-exception error reporting.
 */
class BinaryFileValidator {
 public:
  static constexpr uint32_t MAGIC_NUMBER = 0x54504754;  ///< "TPGT"
  static constexpr uint32_t VERSION = 0x010004;         ///< 1.0.4 in hex
  static constexpr size_t HEADER_SIZE = sizeof(BinaryFileHeader);

  /**
   * @brief Validate a binary file header from a stream.
   *
   * Reads the header from the stream and validates it.
   *
   * @param stream Input stream positioned at the start of the header.
   * @param[out] header Populated with header fields on success.
   * @param[out] error Error message if validation fails.
   * @return true if header is valid, false otherwise.
   */
  static bool validate_stream(std::istream& stream,
                              BinaryFileHeader& header,
                              std::string& error);

  /**
   * @brief Validate a binary file header that already resides in memory.
   *
   * @param data Pointer to header bytes.
   * @param size Number of bytes available at data.
   * @param[out] header Populated with header fields on success.
   * @param[out] error Error message if validation fails.
   * @return true if header is valid, false otherwise.
   */
  static bool validate_buffer(const uint8_t* data,
                              size_t size,
                              BinaryFileHeader& header,
                              std::string& error);

  /**
   * @brief Read and validate a binary file header from a file path.
   *
   * Opens the file, reads the header, and validates it.
   *
   * @param filepath Path to the binary file.
   * @param[out] header Populated with header fields on success.
   * @param[out] error Error message if validation fails (includes file I/O errors).
   * @return true if header is valid, false otherwise.
   */
  static bool validate_file(const std::string& filepath,
                            BinaryFileHeader& header,
                            std::string& error);

 private:
  static bool validate_header_fields(const BinaryFileHeader& header,
                                     std::string& error);
};

} // namespace testapp
} // namespace tpglibs

#include "BinaryFileValidator.hxx"

#endif // TPGLIBS_TESTAPP_BINARYFILEVALIDATOR_HPP_

