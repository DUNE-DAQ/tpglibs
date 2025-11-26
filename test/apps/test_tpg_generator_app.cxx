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
#include "tpglibs/testapp/common/TestConfigParser.hpp"
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
            << " [--validate-config] <input_frames_file> <config_file> <validation_file>" << std::endl;
  std::cerr << "Options:" << std::endl;
  std::cerr << "  --validate-config    Only validate config file, do not run TPGenerator" << std::endl;
  std::cerr << "Example: " << program_name 
            << " frames.bin config.json validation.val" << std::endl;
  std::cerr << "Example: " << program_name 
            << " --validate-config config.json" << std::endl;
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
  bool validate_config_only = false;
  int arg_offset = 1;
  
  // Check for --validate-config flag
  if (argc >= 2 && std::string(argv[1]) == "--validate-config") {
    validate_config_only = true;
    arg_offset = 2;
  }
  
  if (validate_config_only) {
    // Config validation mode: only need config file
    if (argc != 3) {
      print_usage(argv[0]);
      return 1;
    }
  } else {
    // Normal mode: need all three files
    if (argc != 4) {
      print_usage(argv[0]);
      return 1;
    }
  }
  
  std::string input_frames_file = validate_config_only ? "" : argv[arg_offset];
  std::string config_file = argv[validate_config_only ? arg_offset : arg_offset + 1];
  std::string validation_file = validate_config_only ? "" : argv[arg_offset + 2];
  
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
  
  // Parse TPGenerator config using TestConfigParser
  tpglibs::testapp::TPGeneratorTestConfig tpg_config;
  std::string parse_error;
  if (!tpglibs::testapp::TestConfigParser::parse_tpgenerator_config(config, tpg_config, parse_error)) {
    std::cerr << "ERROR: Failed to parse config: " << parse_error << std::endl;
    return 1;
  }
  
  // Frame dimensions are fixed: 64 channels × 256 time samples (matching TPGenerator)
  constexpr int num_channels = 64;
  constexpr int num_time_samples = 256;
  
  // Validate channel-plane mappings size matches fixed num_channels (64)
  if (static_cast<int>(tpg_config.channel_plane_mappings.size()) != num_channels) {
    std::cerr << "ERROR: channel_plane_mappings size (" << tpg_config.channel_plane_mappings.size()
              << ") does not match expected num_channels (64)" << std::endl;
    return 1;
  }
  
  // Config validation mode: print parsed config and exit
  if (validate_config_only) {
    std::cout << "Config validation successful!" << std::endl;
    std::cout << std::endl;
    std::cout << "Parsed configuration:" << std::endl;
    std::cout << "  Processor configs: " << tpg_config.processor_configs.size() << std::endl;
    for (size_t i = 0; i < tpg_config.processor_configs.size(); ++i) {
      std::cout << "    [" << i << "] " << tpg_config.processor_configs[i].first << std::endl;
    }
    std::cout << "  Channel-plane mappings: " << tpg_config.channel_plane_mappings.size() << std::endl;
    std::cout << "  Sample tick difference: " << tpg_config.sample_tick_difference << std::endl;
    std::cout << "  SOT minima: [";
    for (size_t i = 0; i < tpg_config.sot_minima.size(); ++i) {
      std::cout << tpg_config.sot_minima[i];
      if (i < tpg_config.sot_minima.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    std::cout << "  Validation frames: ";
    if (tpg_config.validation_frames.empty()) {
      std::cout << "(none)" << std::endl;
    } else {
      for (size_t i = 0; i < tpg_config.validation_frames.size(); ++i) {
        std::cout << tpg_config.validation_frames[i];
        if (i < tpg_config.validation_frames.size() - 1) std::cout << ", ";
      }
      std::cout << std::endl;
    }
    std::cout << "  Max frames: ";
    if (tpg_config.max_frames > 0) {
      std::cout << tpg_config.max_frames << std::endl;
    } else {
      std::cout << "(unlimited)" << std::endl;
    }
    return 0;
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
  tpglibs::TPGenerator tpg;
  try {
    tpg.configure(tpg_config.processor_configs, tpg_config.channel_plane_mappings, 
                  tpg_config.sample_tick_difference);
    tpg.set_sot_minima(tpg_config.sot_minima);
  } catch (const std::exception& e) {
    std::cerr << "ERROR: Failed to configure TPGenerator: " << e.what() << std::endl;
    return 1;
  }
  
  // Read and process frames (fixed 64×256 dimensions)
  tpglibs::testapp::FrameReader frame_reader(input_frames_file);
  
  std::cout << "Starting TPGenerator processing..." << std::endl;
  std::cout << "Channels: " << num_channels << ", Time samples: " << num_time_samples << std::endl;
  if (!tpg_config.validation_frames.empty()) {
    std::cout << "Validation frames: ";
    for (int frame_idx : tpg_config.validation_frames) {
      std::cout << frame_idx << " ";
    }
    std::cout << std::endl;
  } else {
    std::cout << "No validation frames specified - processing only" << std::endl;
  }
  if (tpg_config.max_frames > 0) {
    std::cout << "Max frames: " << tpg_config.max_frames << std::endl;
  }
  std::cout << std::endl;
  
  // Convert validation_frames vector to set for O(1) lookup
  std::set<int> validation_frames_set(tpg_config.validation_frames.begin(), 
                                       tpg_config.validation_frames.end());
  
  // Track max_frames limit if set
  int max_frames = tpg_config.max_frames > 0 ? tpg_config.max_frames : -1;
  
  size_t frame_index = 0;
  bool all_passed = true;
  
  while (!frame_reader.eof()) {
    // Check max_frames limit if set
    if (max_frames > 0 && static_cast<int>(frame_index) >= max_frames) {
      break;
    }
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
  if (!tpg_config.validation_frames.empty()) {
    std::cout << "Validation results: " << (all_passed ? "PASS" : "FAIL") << std::endl;
    return all_passed ? 0 : 3;
  } else {
    std::cout << "Validation: Not performed (no validation_frames specified)" << std::endl;
    return 0;
  }
}

