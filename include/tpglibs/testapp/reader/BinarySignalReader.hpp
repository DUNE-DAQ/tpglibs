/**
 * @file BinarySignalReader.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_TESTAPP_BINARYSIGNALREADER_HPP_
#define TPGLIBS_TESTAPP_BINARYSIGNALREADER_HPP_

#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>

namespace tpglibs {
namespace testapp {

/**
 * @brief Simple binary file reader for integer types.
 *
 * Reads binary data from a file and returns it as the requested integer type.
 * Used for reading int16_t test data.
 */
template<typename T>
class BinarySignalReader {
  public:
    /**
     * @brief Constructor - opens the file for reading.
     * @param filepath Path to the binary file
     * @throws std::runtime_error if file cannot be opened
     */
    explicit BinarySignalReader(const std::string& filepath);
    
    /**
     * @brief Destructor - closes the file.
     */
    ~BinarySignalReader() = default;
    
    /**
     * @brief Read next chunk of data.
     * @param n Number of elements to read (actual bytes = n * sizeof(T))
     * @return Vector containing the read data
     * @throws std::runtime_error if read fails
     */
    std::vector<T> next(size_t n);
    
    /**
     * @brief Check if we've reached end of file.
     * @return true if EOF reached
     */
    bool eof();
    
    /**
     * @brief Get current file position.
     * @return Current position in file
     */
    std::streampos tellg();
    
    /**
     * @brief Seek to specific position.
     * @param pos Position to seek to
     */
    void seekg(std::streampos pos);

  private:
    std::ifstream m_file;
    bool m_eof_reached;
};

} // namespace testapp
} // namespace tpglibs

#include "BinarySignalReader.hxx"

#endif // TPGLIBS_TESTAPP_BINARYSIGNALREADER_HPP_
