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
#include "tpglibs/testapp/tp/TPValidationReader.hpp"
#include "tpglibs/testapp/tp/TPValidator.hpp"
#include "tpglibs/testapp/common/BinaryFileValidator.hpp"
#include "tpglibs/testapp/common/TestConfigParser.hpp"
#include "tpglibs/TPGenerator.hpp"
#include "trgdataformats/Types.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <optional>
#include <sstream>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>
#include <algorithm>
#include <limits>

// Exit codes
namespace ExitCode {
  constexpr int SUCCESS = 0;
  constexpr int USAGE_ERROR = 1;
  constexpr int FILE_ERROR = 2;
  constexpr int VALIDATION_FAILED = 3;
}

// Frame dimensions (matching TPGenerator and DUMMY_FRAME_STRUCT)
// DUMMY_FRAME_STRUCT is defined in DummyFrameAdapter.hpp
namespace FrameConstants {
  constexpr int NUM_CHANNELS = tpglibs::testapp::DUMMY_FRAME_STRUCT::s_num_channels; // 64
  constexpr int NUM_TIME_SAMPLES = tpglibs::testapp::DUMMY_FRAME_STRUCT::s_time_samples_per_frame; // 256
  constexpr size_t FRAME_HEADER_SIZE = 16;
  constexpr size_t FRAME_DATA_SIZE = NUM_CHANNELS * NUM_TIME_SAMPLES * sizeof(int16_t);
  constexpr size_t EXPECTED_FRAME_SIZE = FRAME_HEADER_SIZE + FRAME_DATA_SIZE;
}

/**
 * @brief Application arguments structure
 */
struct AppArguments {
  bool validate_config_only = false;
  std::string input_frames_file;
  std::string config_file;
  std::string validation_file;
};

/**
 * @brief Print usage information
 */
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
 * @brief Parse command line arguments
 * @return Parsed arguments or empty optional on error
 */
std::optional<AppArguments> parse_arguments(int argc, char* argv[]) {
  AppArguments args;
  
  if (argc < 2) {
    return std::nullopt;
  }
  
  int arg_idx = 1;
  
  // Check for --validate-config flag
  if (std::string(argv[arg_idx]) == "--validate-config") {
    args.validate_config_only = true;
    arg_idx++;
  }
  
  // Validate argument count
  const int required_args = args.validate_config_only ? 1 : 3;
  if (argc != arg_idx + required_args) {
    return std::nullopt;
  }
  
  // Extract file paths
  if (args.validate_config_only) {
    args.config_file = argv[arg_idx++];
  } else {
    args.input_frames_file = argv[arg_idx++];
    args.config_file = argv[arg_idx++];
    args.validation_file = argv[arg_idx++];
  }
  
  return args;
}

/**
 * @brief Load and parse JSON configuration file
 * @return Parsed JSON or empty optional on error
 */
std::optional<nlohmann::json> load_config(const std::string& config_file) {
  std::ifstream config_stream(config_file);
  if (!config_stream.is_open()) {
    std::cerr << "ERROR: Failed to open config file: " << config_file << std::endl;
    std::cerr << "  Check that the file exists and is readable" << std::endl;
    return std::nullopt;
  }
  
  nlohmann::json config;
  try {
    config_stream >> config;
  } catch (const nlohmann::json::parse_error& e) {
    std::cerr << "ERROR: Failed to parse config file (JSON syntax error): " << config_file << std::endl;
    std::cerr << "  Parse error at byte " << e.byte << ": " << e.what() << std::endl;
    return std::nullopt;
  } catch (const std::exception& e) {
    std::cerr << "ERROR: Failed to parse config file: " << config_file << std::endl;
    std::cerr << "  Error: " << e.what() << std::endl;
    return std::nullopt;
  }
  
  return config;
}

/**
 * @brief Validate binary file header
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

/**
 * @brief Format vector for output (comma-separated)
 * Only used for outputting vectors to the console.
 */
template<typename T>
std::string format_vector(const std::vector<T>& vec, const std::string& empty_label = "(none)") {
  if (vec.empty()) {
    return empty_label;
  }
  std::ostringstream oss;
  for (size_t i = 0; i < vec.size(); ++i) {
    oss << vec[i];
    if (i < vec.size() - 1) oss << ", ";
  }
  return oss.str();
}

/**
 * @brief Print parsed configuration (for --validate-config mode)
 */
void print_config_summary(const tpglibs::testapp::TPGeneratorTestConfig& config) {
  std::cout << "Config validation successful!" << std::endl;
  std::cout << std::endl;
  std::cout << "Parsed configuration:" << std::endl;
  std::cout << "  Processor configs: " << config.processor_configs.size() << std::endl;
  for (size_t i = 0; i < config.processor_configs.size(); ++i) {
    std::cout << "    [" << i << "] " << config.processor_configs[i].first << std::endl;
  }
  std::cout << "  Channel-plane mappings: " << config.channel_plane_mappings.size() << std::endl;
  std::cout << "  Sample tick difference: " << config.sample_tick_difference << std::endl;
  std::cout << "  SOT minima: [" << format_vector(config.sot_minima) << "]" << std::endl;
  std::cout << "  Validation frames: " << format_vector(config.validation_frames) << std::endl;
  std::cout << "  Max frames: ";
  if (config.max_frames > 0) {
    std::cout << config.max_frames << std::endl;
  } else {
    std::cout << "(unlimited)" << std::endl;
  }
}

/**
 * @brief Validate channel-plane mappings size
 * @return true if valid, false otherwise
 */
bool validate_channel_mappings(const tpglibs::testapp::TPGeneratorTestConfig& config) {
  if (static_cast<int>(config.channel_plane_mappings.size()) != FrameConstants::NUM_CHANNELS) {
    std::cerr << "ERROR: channel_plane_mappings size (" << config.channel_plane_mappings.size()
              << ") does not match expected num_channels (" << FrameConstants::NUM_CHANNELS << ")" << std::endl;
    return false;
  }
  return true;
}

/**
 * @brief Validate that max_frames covers all validation_frames
 * @return true if valid, false otherwise
 */
bool validate_max_frames_consistency(const tpglibs::testapp::TPGeneratorTestConfig& config) {
  // Only validate if max_frames is set ( > 0) and validation_frames is not empty
  if (config.max_frames > 0 && !config.validation_frames.empty()) {
    int max_validation_frame = *std::max_element(config.validation_frames.begin(), 
                                                  config.validation_frames.end());
    if (config.max_frames <= max_validation_frame) {
      std::cerr << "ERROR: max_frames (" << config.max_frames 
                << ") must be greater than the largest validation frame (" 
                << max_validation_frame << ")" << std::endl;
      std::cerr << "  Validation frames: [" << format_vector(config.validation_frames) << "]" << std::endl;
      std::cerr << "  This ensures all validation frames will be processed" << std::endl;
      return false;
    }
  }
  return true;
}

/**
 * @brief Validate that validation frames exist in validation file
 * @return true if all frames exist, false otherwise
 */
bool validate_validation_frames(const tpglibs::testapp::TPValidationReader& tp_reader,
                                const std::vector<int>& validation_frames) {
  if (validation_frames.empty()) {
    return true;
  }
  
  for (int frame_idx : validation_frames) {
    if (!tp_reader.has_frame(static_cast<uint32_t>(frame_idx))) {
      std::cerr << "ERROR: Validation file does not contain expected TPs for frame " 
                << frame_idx << std::endl;
      std::cerr << "  Available validation frames: " 
                << format_vector(tp_reader.get_validation_frames()) << std::endl;
      return false;
    }
  }
  return true;
}

/**
 * @brief Configure TPGenerator from config
 * @return Configured TPGenerator or empty optional on error
 */
std::optional<tpglibs::TPGenerator> configure_tpgenerator(
    const tpglibs::testapp::TPGeneratorTestConfig& config) {
  tpglibs::TPGenerator tpg;
  try {
    // pass through the config to the TPGenerator
    tpg.configure(config.processor_configs, config.channel_plane_mappings, 
                  config.sample_tick_difference);
    tpg.set_sot_minima(config.sot_minima);
  } catch (const std::exception& e) {
    std::cerr << "ERROR: Failed to configure TPGenerator: " << e.what() << std::endl;
    // assume nothing wrong with TPGenerator itself, then must be something wrong with the config
    std::cerr << "  Check processor_configs and channel_plane_mappings in config file" << std::endl;
    return std::nullopt;
  }
  return tpg;
}

/**
 * @brief Print processing start information
 */
void print_processing_start(const tpglibs::testapp::TPGeneratorTestConfig& config) {
  std::cout << "Starting TPGenerator processing..." << std::endl;
  std::cout << "Channels: " << FrameConstants::NUM_CHANNELS 
            << ", Time samples: " << FrameConstants::NUM_TIME_SAMPLES << std::endl;
  if (!config.validation_frames.empty()) {
    std::cout << "Validation frames: " << format_vector(config.validation_frames, "") << std::endl;
  } else {
    std::cout << "No validation frames specified - processing only" << std::endl;
  }
  if (config.max_frames > 0) {
    std::cout << "Max frames: " << config.max_frames << std::endl;
  }
  std::cout << std::endl;
}

/**
 * @brief Process a single frame through TPGenerator
 * @return Generated TPs or empty optional on error
 */
std::optional<std::vector<dunedaq::trgdataformats::TriggerPrimitive>> process_frame(
    tpglibs::TPGenerator& tpg,
    const tpglibs::testapp::RawFrameView& frame_view,
    size_t frame_index) {
  // Convert to DUMMY_FRAME_STRUCT
  auto frame = tpglibs::testapp::DummyFrameAdapter::create_frame(frame_view);
  if (!frame) {
    std::cerr << "ERROR: Failed to convert frame at index " << frame_index << std::endl;
    std::cerr << "  Frame size: " << frame_view.bytes.size() << " bytes" << std::endl;
    std::cerr << "  Expected: " << FrameConstants::EXPECTED_FRAME_SIZE 
              << " bytes (header + data)" << std::endl;
    if (!tpglibs::testapp::DummyFrameAdapter::validate_frame_view(frame_view)) {
      std::cerr << "  Frame view validation failed - frame size mismatch" << std::endl;
    }
    return std::nullopt;
  }
  
  // Process through TPGenerator
  try {
    // call the TPGenerator
    return tpg(frame.get());
  } catch (const std::exception& e) {
    std::cerr << "ERROR: TPGenerator processing failed at frame " << frame_index << std::endl;
    std::cerr << "  Exception: " << e.what() << std::endl;
    std::cerr << "  This may indicate a problem with processor configuration or frame data" << std::endl;
    return std::nullopt;
  }
}

/**
 * @brief Validate TPs against expected values
 * @return true if validation passed, false otherwise
 */
bool validate_tps(size_t frame_index,
                  const std::vector<dunedaq::trgdataformats::TriggerPrimitive>& expected_tps,
                  const std::vector<dunedaq::trgdataformats::TriggerPrimitive>& actual_tps) {
  std::cout << "Validating frame " << frame_index << "..." << std::endl;
  
  auto validation = tpglibs::testapp::TPValidator::validate(expected_tps, actual_tps);
  
  if (!validation.matches) {
    std::cout << "Validation FAILED for frame " << frame_index << std::endl;
    std::cout << "  " << validation.mismatch_reason << std::endl;
    if (validation.first_mismatch_index != std::numeric_limits<size_t>::max()) {
      std::cout << "  First mismatch at TP index " << validation.first_mismatch_index << std::endl;
      if (validation.first_mismatch_index < expected_tps.size()) {
        std::cout << "  Expected: " 
                  << tpglibs::testapp::TPValidator::format_tp(
                      expected_tps[validation.first_mismatch_index]) << std::endl;
      }
      if (validation.first_mismatch_index < actual_tps.size()) {
        std::cout << "  Actual: " 
                  << tpglibs::testapp::TPValidator::format_tp(
                      actual_tps[validation.first_mismatch_index]) << std::endl;
      }
    }
    return false;
  }
  
  std::cout << "Validation PASSED for frame " << frame_index 
            << " (" << actual_tps.size() << " TPs)" << std::endl;
  return true;
}

/**
 * @brief Process all frames and perform validation
 * @return Exit code (0 = success, 2 = file error, 3 = validation failed)
 */
int process_frames(tpglibs::TPGenerator& tpg,
                    tpglibs::testapp::FrameReader& frame_reader,
                    tpglibs::testapp::TPValidationReader& tp_reader,
                    const tpglibs::testapp::TPGeneratorTestConfig& config) {
  print_processing_start(config);
  
  // Convert validation_frames to set for O(1) lookup
  std::set<int> validation_frames_set(config.validation_frames.begin(), 
                                      config.validation_frames.end());
  
  // Track max_frames limit
  // Use value -1 to indicate no limit
  const int max_frames = config.max_frames > 0 ? config.max_frames : -1;
  
  size_t frame_index = 0;
  bool all_passed = true;
  
  while (!frame_reader.eof()) {
    // Check max_frames limit
    if (max_frames > 0 && static_cast<int>(frame_index) >= max_frames) {
      break;
    }
    
    // Read next frame
    auto [status, frame_view] = frame_reader.next_frame();
    
    if (status == tpglibs::testapp::FrameReadStatus::kEOF) {
      break;
    }
    if (status == tpglibs::testapp::FrameReadStatus::kError) {
      std::cerr << "ERROR: Failed to read frame at index " << frame_index << std::endl;
      std::cerr << "  Possible causes: corrupted frame data, unexpected file size, or I/O error" << std::endl;
      return ExitCode::FILE_ERROR;
    }
    
    // Process frame
    auto actual_tps = process_frame(tpg, frame_view, frame_index);
    if (!actual_tps) {
      return ExitCode::FILE_ERROR;
    }
    
    // Perform requested validation (if this is a validation frame)
    if (validation_frames_set.find(static_cast<int>(frame_index)) != validation_frames_set.end()) {
      auto expected_tps = tp_reader.get_tps_for_frame(frame_index);
      if (!validate_tps(frame_index, expected_tps, *actual_tps)) {
        all_passed = false;
      }
    }
    
    frame_index++;
  }
  
  // Print summary
  std::cout << std::endl << "Processing completed." << std::endl;
  std::cout << "Frames processed: " << frame_index << std::endl;
  
  if (!config.validation_frames.empty()) {
    std::cout << "Validation results: " << (all_passed ? "PASS" : "FAIL") << std::endl;
    return all_passed ? ExitCode::SUCCESS : ExitCode::VALIDATION_FAILED;
  } else {
    std::cout << "Validation: Not performed (no validation_frames specified)" << std::endl;
    return ExitCode::SUCCESS;
  }
}

/**
 * @brief Run config validation mode
 */
int run_config_validation(const std::string& config_file) {
  auto config_json = load_config(config_file);
  if (!config_json) {
    return ExitCode::USAGE_ERROR;
  }
  
  tpglibs::testapp::TPGeneratorTestConfig tpg_config;
  std::string parse_error;
  if (!tpglibs::testapp::TestConfigParser::parse_tpgenerator_config(*config_json, tpg_config, parse_error)) {
    std::cerr << "ERROR: Failed to parse config: " << parse_error << std::endl;
    return ExitCode::USAGE_ERROR;
  }
  
  if (!validate_channel_mappings(tpg_config)) {
    return ExitCode::USAGE_ERROR;
  }
  
  if (!validate_max_frames_consistency(tpg_config)) {
    return ExitCode::USAGE_ERROR;
  }
  
  print_config_summary(tpg_config);
  return ExitCode::SUCCESS;
}

/**
 * @brief Run normal processing mode
 */
int run_processing_mode(const AppArguments& args) {
  // Load and parse config
  auto config_json = load_config(args.config_file);
  if (!config_json) {
    return ExitCode::USAGE_ERROR;
  }
  
  tpglibs::testapp::TPGeneratorTestConfig tpg_config;
  std::string parse_error;
  if (!tpglibs::testapp::TestConfigParser::parse_tpgenerator_config(*config_json, tpg_config, parse_error)) {
    std::cerr << "ERROR: Failed to parse config: " << parse_error << std::endl;
    return ExitCode::USAGE_ERROR;
  }
  
  if (!validate_channel_mappings(tpg_config)) {
    return ExitCode::USAGE_ERROR;
  }
  
  if (!validate_max_frames_consistency(tpg_config)) {
    return ExitCode::USAGE_ERROR;
  }
  
  // Validate binary file headers
  if (!validate_binary_file(args.input_frames_file, "input frames")) {
    return ExitCode::FILE_ERROR;
  }
  if (!validate_binary_file(args.validation_file, "validation")) {
    return ExitCode::FILE_ERROR;
  }
  
  // Read expected TPs from validation file
  tpglibs::testapp::TPValidationReader tp_reader(args.validation_file);
  // Note: TPValidationReader constructor throws std::runtime_error on failure
  
  // Validate that validation frames exist
  if (!validate_validation_frames(tp_reader, tpg_config.validation_frames)) {
    return ExitCode::FILE_ERROR;
  }
  
  // Configure TPGenerator
  auto tpg = configure_tpgenerator(tpg_config);
  if (!tpg) {
    return ExitCode::USAGE_ERROR;
  }
  
  // Read and process frames
  tpglibs::testapp::FrameReader frame_reader(args.input_frames_file);
  // Note: FrameReader constructor throws std::runtime_error on failure
  
  return process_frames(*tpg, frame_reader, tp_reader, tpg_config);
}

int main(int argc, char* argv[]) {
  auto args = parse_arguments(argc, argv);
  if (!args) {
    print_usage(argv[0]);
    return ExitCode::USAGE_ERROR;
  }
  
  if (args->validate_config_only) {
    return run_config_validation(args->config_file);
  } else {
    return run_processing_mode(*args);
  }
}
