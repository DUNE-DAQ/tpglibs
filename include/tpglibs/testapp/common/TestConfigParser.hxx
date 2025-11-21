/**
 * @file TestConfigParser.hxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_TESTAPP_TESTCONFIGPARSER_HXX_
#define TPGLIBS_TESTAPP_TESTCONFIGPARSER_HXX_

#include "TestConfigParser.hpp"
#include <algorithm>

namespace tpglibs {
namespace testapp {

bool TestConfigParser::parse_processor_config(const nlohmann::json& config,
                                              ProcessorTestConfig& result,
                                              std::string& error) {
  // Validate test_config section
  if (!validate_required_object(config, "test_config", error)) {
    return false;
  }
  const auto& test_config = config["test_config"];
  
  // Validate processor_name
  if (!validate_required_string(test_config, "processor_name", error)) {
    return false;
  }
  result.processor_name = test_config["processor_name"].get<std::string>();
  
  // Validate samples_per_time_step
  if (!validate_required_integer(test_config, "samples_per_time_step", error)) {
    return false;
  }
  result.samples_per_time_step = test_config["samples_per_time_step"].get<int>();
  if (result.samples_per_time_step <= 0) {
    error = "test_config.samples_per_time_step must be a positive integer";
    return false;
  }
  
  // Parse validation_steps (optional)
  result.validation_steps.clear();
  if (test_config.contains("validation_steps")) {
    if (!test_config["validation_steps"].is_array()) {
      error = "test_config.validation_steps must be an array if provided";
      return false;
    }
    for (const auto& step : test_config["validation_steps"]) {
      if (!step.is_number_integer()) {
        error = "test_config.validation_steps must contain integers";
        return false;
      }
      int step_value = step.get<int>();
      if (step_value < 0) {
        error = "test_config.validation_steps must contain non-negative integers";
        return false;
      }
      result.validation_steps.push_back(step_value);
    }
  }
  
  // Parse max_steps (optional, but required if validation_steps is empty)
  if (test_config.contains("max_steps")) {
    if (!test_config["max_steps"].is_number_integer()) {
      error = "test_config.max_steps must be an integer if provided";
      return false;
    }
    result.max_steps = test_config["max_steps"].get<int>();
    if (result.max_steps <= 0) {
      error = "test_config.max_steps must be a positive integer";
      return false;
    }
  } else {
    // Derive max_steps from validation_steps
    if (result.validation_steps.empty()) {
      error = "Either test_config.max_steps or non-empty test_config.validation_steps must be provided";
      return false;
    }
    int derived_max = 0;
    for (int step : result.validation_steps) {
      derived_max = std::max(derived_max, step);
    }
    result.max_steps = derived_max + 1;
  }
  
  // Validate processor_config section
  if (!validate_required_object(config, "processor_config", error)) {
    return false;
  }
  result.processor_config = config["processor_config"];
  
  return true;
}

bool TestConfigParser::validate_required_object(const nlohmann::json& obj,
                                                 const std::string& field_name,
                                                 std::string& error) {
  if (!obj.contains(field_name)) {
    error = "Missing " + field_name + " in config";
    return false;
  }
  if (!obj[field_name].is_object()) {
    error = field_name + " must be an object";
    return false;
  }
  return true;
}

bool TestConfigParser::validate_required_string(const nlohmann::json& obj,
                                                 const std::string& field_name,
                                                 std::string& error) {
  if (!obj.contains(field_name)) {
    error = "Missing " + field_name + " field";
    return false;
  }
  if (!obj[field_name].is_string()) {
    error = field_name + " must be a string";
    return false;
  }
  return true;
}

bool TestConfigParser::validate_required_integer(const nlohmann::json& obj,
                                                  const std::string& field_name,
                                                  std::string& error) {
  if (!obj.contains(field_name)) {
    error = "Missing " + field_name + " field";
    return false;
  }
  if (!obj[field_name].is_number_integer()) {
    error = field_name + " must be an integer";
    return false;
  }
  return true;
}

} // namespace testapp
} // namespace tpglibs

#endif // TPGLIBS_TESTAPP_TESTCONFIGPARSER_HXX_

