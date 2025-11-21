/**
 * @file test_tpg_generator_app.cxx
 *
 * @brief TPGenerator Test Application - Processes frames through TPGenerator and validates TPs
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/testapp/frame/FrameReader.hpp"
#include "tpglibs/testapp/frame/DummyFrameAdapter.hpp"
#include "tpglibs/testapp/tp/TPReader.hpp"
#include "tpglibs/testapp/tp/TPComparator.hpp"
#include "tpglibs/testapp/common/BinaryFileValidator.hpp"
#include "tpglibs/TPGenerator.hpp"
#include "trgdataformats/Types.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>
#include <algorithm>
#include <limits>

void print_usage(const char* program_name) {
  std::cerr << "Usage: " << program_name 
            << " <input_frames_file> <config_file> <validation_file>" << std::endl;
  std::cerr << "Example: " << program_name 
            << " frames.bin config.json validation.val" << std::endl;
}

/**
 * @brief Validate binary file header
 * @param filepath Path to file
 * @param label Label for error messages (e.g., "input frames", "validation")
 * @return true if valid, false otherwise
 */
bool validate_binary_file(const std::string& filepath, const std::string& label) {
  tpglibs::testapp::BinaryFileHeader header;
  std::string error;
  if (!tpglibs::testapp::BinaryFileValidator::validate_file(filepath, header, error)) {
    std::cerr << "ERROR: Invalid " << label << " file: " << error << std::endl;
    return false;
  }
  return true;
}

int main(int argc, char* argv[]) {
  if (argc != 4) {
    print_usage(argv[0]);
    return 1;
  }
  
  std::string input_frames_file = argv[1];
  std::string config_file = argv[2];
  std::string validation_file = argv[3];
  
  // Load configuration
  std::ifstream config_stream(config_file);
  if (!config_stream.is_open()) {
    std::cerr << "ERROR: Failed to open config file: " << config_file << std::endl;
    return 1;
  }
  
  nlohmann::json config;
  try {
    config_stream >> config;
  } catch (const std::exception& e) {
    std::cerr << "ERROR: Failed to parse config file: " << e.what() << std::endl;
    return 1;
  }
  config_stream.close();
  
  // Parse basic config
  // Frame dimensions are fixed: 64 channels × 256 time samples (matching TPGenerator)
  constexpr int num_channels = 64;
  constexpr int num_time_samples = 256;
  float sample_tick_difference = 1.0;
  std::vector<uint16_t> sot_minima = {1, 1, 1};
  std::vector<int> validation_frames;
  
  if (config.contains("test_config")) {
    const auto& test_config = config["test_config"];
    if (test_config.contains("sample_tick_difference")) {
      sample_tick_difference = test_config["sample_tick_difference"].get<float>();
    }
    if (test_config.contains("sot_minima") && test_config["sot_minima"].is_array()) {
      sot_minima.clear();
      for (const auto& val : test_config["sot_minima"]) {
        if (!val.is_number_unsigned()) {
          std::cerr << "ERROR: sot_minima must contain non-negative integers" << std::endl;
          return 1;
        }
        sot_minima.push_back(val.get<uint16_t>());
      }
      // Validate sot_minima has reasonable size (typically 3 for 3 planes)
      if (sot_minima.empty()) {
        std::cerr << "ERROR: sot_minima cannot be empty" << std::endl;
        return 1;
      }
    }
    if (test_config.contains("validation_frames") && test_config["validation_frames"].is_array()) {
      for (const auto& val : test_config["validation_frames"]) {
        if (!val.is_number_integer()) {
          std::cerr << "ERROR: validation_frames must contain integers" << std::endl;
          return 1;
        }
        int frame_idx = val.get<int>();
        if (frame_idx < 0) {
          std::cerr << "ERROR: validation_frames must contain non-negative integers" << std::endl;
          return 1;
        }
        validation_frames.push_back(frame_idx);
      }
    }
  }
  
  // Parse processor configs (minimal for now)
  std::vector<std::pair<std::string, nlohmann::json>> processor_configs;
  if (config.contains("processor_configs") && config["processor_configs"].is_array()) {
    for (const auto& proc_config : config["processor_configs"]) {
      if (proc_config.contains("processor_name") && proc_config.contains("config")) {
        processor_configs.push_back({
          proc_config["processor_name"].get<std::string>(),
          proc_config["config"]
        });
      }
    }
  }
  
  // Parse channel-plane mappings
  std::vector<std::pair<dunedaq::trgdataformats::channel_t, int16_t>> channel_plane_mappings;
  if (config.contains("channel_plane_mappings") && config["channel_plane_mappings"].is_array()) {
    for (const auto& mapping : config["channel_plane_mappings"]) {
      if (mapping.is_array() && mapping.size() == 2) {
        channel_plane_mappings.push_back({
          mapping[0].get<dunedaq::trgdataformats::channel_t>(),
          mapping[1].get<int16_t>()
        });
      }
    }
  }
  
  // Validate channel-plane mappings size matches fixed num_channels (64)
  if (!channel_plane_mappings.empty() && 
      static_cast<int>(channel_plane_mappings.size()) != num_channels) {
    std::cerr << "ERROR: channel_plane_mappings size (" << channel_plane_mappings.size()
              << ") does not match expected num_channels (64)" << std::endl;
    return 1;
  }
  
  // Validate binary file headers
  if (!validate_binary_file(input_frames_file, "input frames")) {
    return 2;
  }
  if (!validate_binary_file(validation_file, "validation")) {
    return 2;
  }
  
  // Read expected TPs from validation file
  tpglibs::testapp::TPReader tp_reader(validation_file);
  
  // Configure TPGenerator
  if (processor_configs.empty()) {
    std::cerr << "ERROR: No processor configs provided" << std::endl;
    return 1;
  }
  if (channel_plane_mappings.empty()) {
    std::cerr << "ERROR: No channel-plane mappings provided" << std::endl;
    return 1;
  }
  
  tpglibs::TPGenerator tpg;
  try {
    tpg.configure(processor_configs, channel_plane_mappings, sample_tick_difference);
    tpg.set_sot_minima(sot_minima);
  } catch (const std::exception& e) {
    std::cerr << "ERROR: Failed to configure TPGenerator: " << e.what() << std::endl;
    return 1;
  }
  
  // Read and process frames (fixed 64×256 dimensions)
  tpglibs::testapp::FrameReader frame_reader(input_frames_file);
  
  std::cout << "Starting TPGenerator processing..." << std::endl;
  std::cout << "Channels: " << num_channels << ", Time samples: " << num_time_samples << std::endl;
  if (!validation_frames.empty()) {
    std::cout << "Validation frames: ";
    for (int frame_idx : validation_frames) {
      std::cout << frame_idx << " ";
    }
    std::cout << std::endl;
  } else {
    std::cout << "No validation frames specified - processing only" << std::endl;
  }
  std::cout << std::endl;
  
  // Convert validation_frames vector to set for O(1) lookup
  std::set<int> validation_frames_set(validation_frames.begin(), validation_frames.end());
  
  size_t frame_index = 0;
  bool all_passed = true;
  
  while (!frame_reader.eof()) {
    auto [status, frame_view] = frame_reader.next_frame();
    
    if (status == tpglibs::testapp::FrameReadStatus::END_OF_FILE) {
      break;
    }
    if (status == tpglibs::testapp::FrameReadStatus::ERROR) {
      std::cerr << "ERROR: Failed to read frame at index " << frame_index << std::endl;
      return 2;
    }
    
    // Convert to DUMMY_FRAME_STRUCT (directly compatible with TPGenerator)
    auto frame = tpglibs::testapp::DummyFrameAdapter::create_frame(frame_view);
    if (!frame) {
      std::cerr << "ERROR: Failed to convert frame at index " << frame_index << std::endl;
      return 2;
    }
    
    // Process through TPGenerator (DUMMY_FRAME_STRUCT is directly compatible)
    std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual_tps;
    try {
      actual_tps = tpg(frame.get());
    } catch (const std::exception& e) {
      std::cerr << "ERROR: TPGenerator processing failed at frame " << frame_index 
                << ": " << e.what() << std::endl;
      return 2;
    }
    
    // Validate if this is a validation frame (use set for O(1) lookup)
    if (validation_frames_set.find(static_cast<int>(frame_index)) != validation_frames_set.end()) {
      std::cout << "Validating frame " << frame_index << "..." << std::endl;
      
      auto expected_tps = tp_reader.get_tps_for_frame(frame_index);
      auto comparison = tpglibs::testapp::TPComparator::compare(expected_tps, actual_tps);
      
      if (!comparison.matches) {
        std::cout << "Validation FAILED for frame " << frame_index << std::endl;
        std::cout << "  " << comparison.mismatch_reason << std::endl;
        if (comparison.first_mismatch_index != std::numeric_limits<size_t>::max()) {
          std::cout << "  First mismatch at TP index " << comparison.first_mismatch_index << std::endl;
          if (comparison.first_mismatch_index < expected_tps.size()) {
            std::cout << "  Expected: " 
                      << tpglibs::testapp::TPComparator::format_tp(
                          expected_tps[comparison.first_mismatch_index]) << std::endl;
          }
          if (comparison.first_mismatch_index < actual_tps.size()) {
            std::cout << "  Actual: " 
                      << tpglibs::testapp::TPComparator::format_tp(
                          actual_tps[comparison.first_mismatch_index]) << std::endl;
          }
        }
        all_passed = false;
      } else {
        std::cout << "Validation PASSED for frame " << frame_index 
                  << " (" << actual_tps.size() << " TPs)" << std::endl;
      }
    }
    
    frame_index++;
  }
  
  std::cout << std::endl << "Processing completed." << std::endl;
  std::cout << "Frames processed: " << frame_index << std::endl;
  if (!validation_frames.empty()) {
    std::cout << "Validation results: " << (all_passed ? "PASS" : "FAIL") << std::endl;
    return all_passed ? 0 : 3;
  } else {
    std::cout << "Validation: Not performed (no validation_frames specified)" << std::endl;
    return 0;
  }
}

