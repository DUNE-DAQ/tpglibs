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
};

} // namespace testapp
} // namespace tpglibs

#include "TestConfigParser.hxx"

#endif // TPGLIBS_TESTAPP_TESTCONFIGPARSER_HPP_

