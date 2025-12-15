/**
 * @file TestConfigParser.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/testapp/common/TestConfigParser.hpp"
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

bool TestConfigParser::parse_tpgenerator_config(const nlohmann::json& config,
                                                 TPGeneratorTestConfig& result,
                                                 std::string& error) {
  // Initialize defaults
  result.sample_tick_difference = 1.0f;
  result.sot_minima = {1, 1, 1};
  result.validation_frames.clear();
  result.max_frames = -1;  // -1 means not set
  result.processor_configs.clear();
  result.channel_plane_mappings.clear();
  
  // Parse processor_configs (required, non-empty)
  if (!validate_required_array(config, "processor_configs", error)) {
    return false;
  }
  
  for (const auto& proc_config : config["processor_configs"]) {
    if (!proc_config.is_object()) {
      error = "Each element in processor_configs must be an object";
      return false;
    }
    if (!validate_required_string(proc_config, "processor_name", error)) {
      return false;
    }
    if (!validate_required_object(proc_config, "config", error)) {
      return false;
    }
    result.processor_configs.push_back({
      proc_config["processor_name"].get<std::string>(),
      proc_config["config"]
    });
  }
  
  // Parse channel_plane_mappings (required, non-empty)
  if (!validate_required_array(config, "channel_plane_mappings", error)) {
    return false;
  }
  
  for (const auto& mapping : config["channel_plane_mappings"]) {
    if (!mapping.is_array() || mapping.size() != 2) {
      error = "Each channel_plane_mapping must be an array of 2 elements [channel, plane]";
      return false;
    }
    // Accept both signed and unsigned integers for channel (nlohmann::json stores small ints as signed)
    if (!mapping[0].is_number() || !mapping[1].is_number_integer()) {
      error = "channel_plane_mapping must be [number, integer]";
      return false;
    }
    // Ensure channel is non-negative
    if (mapping[0].get<int64_t>() < 0) {
      error = "channel_plane_mapping channel must be non-negative";
      return false;
    }
    dunedaq::trgdataformats::channel_t channel = mapping[0].get<dunedaq::trgdataformats::channel_t>();
    int16_t plane = mapping[1].get<int16_t>();
    if (plane < 0 || plane > 2) {
      error = "channel_plane_mapping plane must be 0, 1, or 2";
      return false;
    }
    result.channel_plane_mappings.push_back({channel, plane});
  }
  
  // Parse test_config (optional, but if present validate structure)
  if (config.contains("test_config")) {
    if (!config["test_config"].is_object()) {
      error = "test_config must be an object if provided";
      return false;
    }
    const auto& test_config = config["test_config"];
    
    // Parse sample_tick_difference (optional, default 1.0)
    if (test_config.contains("sample_tick_difference")) {
      if (!test_config["sample_tick_difference"].is_number()) {
        error = "test_config.sample_tick_difference must be a number";
        return false;
      }
      result.sample_tick_difference = test_config["sample_tick_difference"].get<float>();
      if (result.sample_tick_difference <= 0.0f) {
        error = "test_config.sample_tick_difference must be positive";
        return false;
      }
    }
    
    // Parse sot_minima (optional, default [1,1,1])
    if (test_config.contains("sot_minima")) {
      if (!test_config["sot_minima"].is_array()) {
        error = "test_config.sot_minima must be an array if provided";
        return false;
      }
      result.sot_minima.clear();
      for (const auto& val : test_config["sot_minima"]) {
        if (!val.is_number_integer()) {
          error = "test_config.sot_minima must contain integers";
          return false;
        }
        int64_t int_val = val.get<int64_t>();
        if (int_val < 0 || int_val > std::numeric_limits<uint16_t>::max()) {
          error = "test_config.sot_minima must contain non-negative integers within uint16_t range";
          return false;
        }
        result.sot_minima.push_back(static_cast<uint16_t>(int_val));
      }
      if (result.sot_minima.empty()) {
        error = "test_config.sot_minima cannot be empty";
        return false;
      }
    }
    
    // Parse validation_frames (optional)
    if (test_config.contains("validation_frames")) {
      if (!test_config["validation_frames"].is_array()) {
        error = "test_config.validation_frames must be an array if provided";
        return false;
      }
      result.validation_frames.clear();
      for (const auto& val : test_config["validation_frames"]) {
        if (!val.is_number_integer()) {
          error = "test_config.validation_frames must contain integers";
          return false;
        }
        int frame_idx = val.get<int>();
        if (frame_idx < 0) {
          error = "test_config.validation_frames must contain non-negative integers";
          return false;
        }
        result.validation_frames.push_back(frame_idx);
      }
    }
    
    // Parse max_frames (optional, derived from validation_frames if not provided)
    if (test_config.contains("max_frames")) {
      if (!test_config["max_frames"].is_number_integer()) {
        error = "test_config.max_frames must be an integer if provided";
        return false;
      }
      result.max_frames = test_config["max_frames"].get<int>();
      if (result.max_frames <= 0) {
        error = "test_config.max_frames must be a positive integer";
        return false;
      }
    } else {
      // Derive max_frames from validation_frames if available
      if (!result.validation_frames.empty()) {
        int derived_max = 0;
        for (int frame_idx : result.validation_frames) {
          derived_max = std::max(derived_max, frame_idx);
        }
        result.max_frames = derived_max + 1;
      }
      // If no validation_frames and no max_frames, max_frames remains -1 (unlimited)
    }
  }
  
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

bool TestConfigParser::validate_required_array(const nlohmann::json& obj,
                                                const std::string& field_name,
                                                std::string& error) {
  if (!obj.contains(field_name)) {
    error = "Missing " + field_name + " in config";
    return false;
  }
  if (!obj[field_name].is_array()) {
    error = field_name + " must be an array";
    return false;
  }
  if (obj[field_name].empty()) {
    error = field_name + " must be non-empty";
    return false;
  }
  return true;
}

} // namespace testapp
} // namespace tpglibs

