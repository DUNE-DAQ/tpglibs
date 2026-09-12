/**
 * @file TestConfigParser.hpp
 *
 * @brief Parser for test application JSON configuration files
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_TESTAPP_TESTCONFIGPARSER_HPP_
#define TPGLIBS_TESTAPP_TESTCONFIGPARSER_HPP_

#include <nlohmann/json.hpp>
#include "trgdataformats/Types.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace tpglibs {
namespace testapp {

/**
 * @brief Parsed test configuration for processor test application
 */
struct ProcessorTestConfig {
  std::string processor_name;              ///< Processor class name (required)
  int samples_per_time_step;                ///< Number of samples per time step (required)
  std::vector<int> validation_steps;       ///< Step indices to validate (optional, empty if not provided)
  int max_steps;                            ///< Maximum steps to process (required, derived from validation_steps if not provided)
  nlohmann::json processor_config;          ///< Processor-specific configuration (required)
};

/**
 * @brief Parsed test configuration for TPGenerator test application
 */
struct TPGeneratorTestConfig {
  std::vector<std::pair<std::string, nlohmann::json>> processor_configs;  ///< Processor configs (required, non-empty)
  std::vector<std::pair<dunedaq::trgdataformats::channel_t, int16_t>> channel_plane_mappings;  ///< Channel-plane mappings (required, non-empty)
  float sample_tick_difference;                                           ///< Sample tick difference (default: 1.0)
  std::vector<uint16_t> sot_minima;                                       ///< Samples over threshold minima per plane (default: [1,1,1])
  std::vector<int> validation_frames;                                     ///< Frame indices to validate (optional, empty if not provided)
  int max_frames;                                                          ///< Maximum frames to process (optional, derived from validation_frames if not provided)
};

/**
 * @brief Parser for test application JSON configuration files
 *
 * Parses and validates JSON configuration files. Uses bool + error string
 * for error reporting (no exceptions) for clear error messages.
 */
class TestConfigParser {
 public:
  /**
   * @brief Parse processor test configuration
   *
   * Parses JSON config for processor test application format:
   * {
   *   "processor_config": { ... },
   *   "test_config": {
   *     "processor_name": "...",
   *     "samples_per_time_step": 16,
   *     "validation_steps": [1, 200000],
   *     "max_steps": 250000  // optional
   *   }
   * }
   *
   * @param config JSON configuration object
   * @param[out] result Parsed configuration (only valid if function returns true)
   * @param[out] error Error message if parsing fails (only valid if function returns false)
   * @return true if parsing succeeded, false otherwise
   */
  static bool parse_processor_config(const nlohmann::json& config,
                                     ProcessorTestConfig& result,
                                     std::string& error);

  /**
   * @brief Parse TPGenerator test configuration
   *
   * Parses JSON config for TPGenerator test application format:
   * {
   *   "processor_configs": [
   *     {
   *       "processor_name": "...",
   *       "config": { ... }
   *     }
   *   ],
   *   "test_config": {
   *     "sample_tick_difference": 1.0,  // optional, default 1.0
   *     "sot_minima": [1, 1, 1],         // optional, default [1,1,1]
   *     "validation_frames": [0, 10],    // optional
   *     "max_frames": 100                 // optional, derived from validation_frames if not provided
   *   },
   *   "channel_plane_mappings": [
   *     [0, 0], [1, 0], ...  // [channel, plane] pairs
   *   ]
   * }
   *
   * @param config JSON configuration object
   * @param[out] result Parsed configuration (only valid if function returns true)
   * @param[out] error Error message if parsing fails (only valid if function returns false)
   * @return true if parsing succeeded, false otherwise
   */
  static bool parse_tpgenerator_config(const nlohmann::json& config,
                                       TPGeneratorTestConfig& result,
                                       std::string& error);

 private:
  /**
   * @brief Validate that JSON value is a required object field
   * @param obj JSON object
   * @param field_name Field name to check
   * @param[out] error Error message if validation fails
   * @return true if field exists and is an object, false otherwise
   */
  static bool validate_required_object(const nlohmann::json& obj,
                                       const std::string& field_name,
                                       std::string& error);
  
  /**
   * @brief Validate that JSON value is a required string field
   * @param obj JSON object
   * @param field_name Field name to check
   * @param[out] error Error message if validation fails
   * @return true if field exists and is a string, false otherwise
   */
  static bool validate_required_string(const nlohmann::json& obj,
                                        const std::string& field_name,
                                        std::string& error);
  
  /**
   * @brief Validate that JSON value is a required integer field
   * @param obj JSON object
   * @param field_name Field name to check
   * @param[out] error Error message if validation fails
   * @return true if field exists and is an integer, false otherwise
   */
  static bool validate_required_integer(const nlohmann::json& obj,
                                         const std::string& field_name,
                                         std::string& error);
  
  /**
   * @brief Validate that JSON value is a required non-empty array field
   * @param obj JSON object
   * @param field_name Field name to check
   * @param[out] error Error message if validation fails
   * @return true if field exists, is an array, and is non-empty, false otherwise
   */
  static bool validate_required_array(const nlohmann::json& obj,
                                      const std::string& field_name,
                                      std::string& error);
};

} // namespace testapp
} // namespace tpglibs

#endif // TPGLIBS_TESTAPP_TESTCONFIGPARSER_HPP_

