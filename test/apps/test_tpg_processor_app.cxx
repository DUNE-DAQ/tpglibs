/**
 * @file test_tpg_processor_app.cxx
 *
 * @brief TPG Processor Test Application - Processes binary data through configured processors
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/testapp/common/BinaryFileValidator.hpp"
#include "tpglibs/testapp/reader/BinarySignalReader.hpp"
#include "tpglibs/AVXFactory.hpp"
#include "tpglibs/NaiveFactory.hpp"
#include "tpglibs/AbstractProcessor.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <algorithm>

std::shared_ptr<tpglibs::AbstractProcessor<std::array<int16_t, 16>>> 
create_naive_processor(const std::string& processor_name) {
    auto factory = tpglibs::NaiveFactory::get_instance();
    auto processor = factory->create_processor(processor_name);
    return processor;
}

std::shared_ptr<tpglibs::AVXProcessor>
create_avx_processor(const std::string& processor_name) {
    auto factory = tpglibs::AVXFactory::get_instance();
    auto processor = factory->create_processor(processor_name);
    return processor;
}

std::array<int16_t, 16> convert_vector_to_array(const std::vector<int16_t>& vec) {
    std::array<int16_t, 16> result;
    std::fill(result.begin(), result.end(), 0);
    std::copy(vec.begin(), vec.begin() + std::min(vec.size(), size_t(16)), result.begin());
    return result;
}

std::vector<int16_t> convert_array_to_vector(const std::array<int16_t, 16>& arr) {
    return std::vector<int16_t>(arr.begin(), arr.end());
}

__m256i convert_array_i16x16_to_m256(const std::array<int16_t, 16>& arr) {
    return _mm256_lddqu_si256(reinterpret_cast<const __m256i*>(arr.data()));
}

std::array<int16_t, 16> convert_m256_to_array_i16x16(const __m256i& avx_val) {
    std::array<int16_t, 16> result;
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(result.data()), avx_val);
    return result;
}

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name 
              << " <input_file> <config_file> <validation_file>" << std::endl;
    std::cerr << "Example: " << program_name 
              << " test_input.bin config.json validation.bin" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        print_usage(argv[0]);
        return 1;
    }
    
    std::string input_file = argv[1];
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
    
    // Validate input file
    std::ifstream input_stream(input_file, std::ios::binary);
    if (!input_stream.is_open()) {
        std::cerr << "ERROR: Failed to open input file: " << input_file << std::endl;
        return 1;
    }
    
    auto validate_binary_file = [](std::ifstream& stream,
                                   const std::string& path,
                                   const std::string& label) {
        tpglibs::testapp::BinaryFileHeader header;
        std::string error;
        if (!tpglibs::testapp::BinaryFileValidator::validate_stream(stream, header, error)) {
            std::cerr << "ERROR: Invalid " << label << " file header for " << path;
            if (!error.empty()) {
                std::cerr << ": " << error;
            }
            std::cerr << std::endl;
            return false;
        }
        return true;
    };
    
    if (!validate_binary_file(input_stream, input_file, "input")) {
        return 1;
    }
    
    // Validate validation file
    std::ifstream validation_stream(validation_file, std::ios::binary);
    if (!validation_stream.is_open()) {
        std::cerr << "ERROR: Failed to open validation file: " << validation_file << std::endl;
        return 1;
    }
    
    if (!validate_binary_file(validation_stream, validation_file, "validation")) {
        return 1;
    }
    
    // Test config validation block
    if (!config.contains("test_config") || !config["test_config"].is_object()) {
        std::cerr << "ERROR: Missing test_config section in config" << std::endl;
        return 1;
    }
    const auto& test_config = config["test_config"];

    if (!test_config.contains("processor_name") || !test_config["processor_name"].is_string()) {
        std::cerr << "ERROR: test_config.processor_name must be a string" << std::endl;
        return 1;
    }
    std::string processor_name = test_config["processor_name"].get<std::string>();

    if (!test_config.contains("samples_per_time_step") || !test_config["samples_per_time_step"].is_number_integer()) {
        std::cerr << "ERROR: test_config.samples_per_time_step must be an integer" << std::endl;
        return 1;
    }
    int samples_per_time_step = test_config["samples_per_time_step"].get<int>();

    auto validation_steps = test_config.value("validation_steps", nlohmann::json::array());
    if (!validation_steps.is_array()) {
        std::cerr << "ERROR: test_config.validation_steps must be an array if provided" << std::endl;
        return 1;
    }

    int max_steps = 0;
    if (test_config.contains("max_steps") && test_config["max_steps"].is_number_integer()) {
        max_steps = test_config["max_steps"].get<int>();
        if (max_steps <= 0) {
            std::cerr << "ERROR: test_config.max_steps must be a positive integer" << std::endl;
            return 1;
        }
    } else {
        if (validation_steps.empty()) {
            std::cerr << "ERROR: Either test_config.max_steps or non-empty test_config.validation_steps must be provided" << std::endl;
            return 1;
        }
        int derived_max = 0;
        for (const auto& v : validation_steps) {
            if (!v.is_number_integer()) {
                std::cerr << "ERROR: validation_steps must contain integers" << std::endl;
                return 1;
            }
            derived_max = std::max(derived_max, v.get<int>());
        }
        max_steps = derived_max + 1;
    }
    
    // Create processor based on name
    std::shared_ptr<tpglibs::AVXProcessor> avx_processor;
    std::shared_ptr<tpglibs::AbstractProcessor<std::array<int16_t, 16>>> naive_processor;
    bool is_avx = (processor_name.find("AVX") != std::string::npos);
    
    try {
        if (is_avx) {
            avx_processor = create_avx_processor(processor_name);
        } else {
            naive_processor = create_naive_processor(processor_name);
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Failed to create processor: " << e.what() << std::endl;
        return 1;
    }
    
    // Configure processor
    int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};
    nlohmann::json processor_config = config["processor_config"];
    
    if (is_avx) {
        avx_processor->configure(processor_config, plane_numbers);
    } else {
        naive_processor->configure(processor_config, plane_numbers);
    }
    
    // Read binary data
    tpglibs::testapp::BinarySignalReader<int16_t> reader(input_file);
    // Skip the header (12 bytes)
    reader.seekg(12);
    
    // Read validation data only if validation_steps is provided
    std::vector<std::vector<int16_t>> validation_data;
    bool has_validation = !validation_steps.empty();
    
    if (has_validation) {
        // Read expected outputs by seeking to the correct step offset in the validation file
        validation_data.resize(validation_steps.size());
        const std::streamsize step_bytes = static_cast<std::streamsize>(samples_per_time_step * sizeof(int16_t));
        const std::streamoff header_size =
            static_cast<std::streamoff>(tpglibs::testapp::BinaryFileValidator::s_header_size);

        for (size_t i = 0; i < validation_steps.size(); ++i) {
            validation_data[i].resize(samples_per_time_step);
            int target_step = validation_steps.at(i).get<int>();
            std::streamoff offset = header_size + static_cast<std::streamoff>(target_step) * step_bytes;
            
            validation_stream.clear();
            validation_stream.seekg(offset, std::ios::beg);
            validation_stream.read(reinterpret_cast<char*>(validation_data[i].data()), step_bytes);
            
            if (validation_stream.gcount() < step_bytes) {
                std::cout << "WARNING: Validation file does not contain data for step " << target_step 
                          << ". Loaded " << i << "/" << validation_steps.size() << " validation records." << std::endl;
                validation_data.resize(i);
                break;
            }
        }
    }
    
    // Process data step by step
    int step = 0;
    size_t validation_index = 0;
    bool all_passed = true;
    size_t num_steps_processed = 0;
    
    std::cout << "Starting processing..." << std::endl;
    std::cout << "Processor: " << processor_name << std::endl;
    std::cout << "Max steps: " << max_steps << std::endl;
    if (has_validation) {
        std::cout << "Validation steps: ";
        for (auto step_num : validation_steps) {
            std::cout << step_num << " ";
        }
        std::cout << std::endl;
    } else {
        std::cout << "Validation: Disabled (no validation_steps provided)" << std::endl;
    }
    std::cout << std::endl;
    
    while (step < max_steps && !reader.eof()) {
        // Read one time step of data
        auto time_step_data = reader.next(samples_per_time_step);
        if (time_step_data.empty()) break;
        
        // Ensure we have exactly 16 samples
        time_step_data.resize(16, 0);
        
        // Convert to array
        std::array<int16_t, 16> input_array = convert_vector_to_array(time_step_data);
        
        // Process the time step
        std::array<int16_t, 16> result;
        if (is_avx) {
            __m256i input_avx = convert_array_i16x16_to_m256(input_array);
            __m256i result_avx = avx_processor->process(input_avx);
            result = convert_m256_to_array_i16x16(result_avx);
        } else {
            result = naive_processor->process(input_array);
        }
        
        // Check if this is a validation step
        if (validation_index < validation_data.size() &&
            step == validation_steps.at(validation_index).get<int>()) {
            std::vector<int16_t>& expected = validation_data[validation_index];
            expected.resize(16, 0);

            // Find first mismatch using std::mismatch for clarity
            auto mm = std::mismatch(result.begin(), result.end(), expected.begin());
            bool passed = (mm.first == result.end());

            if (!passed) {
                if (all_passed) {
                    int idx = static_cast<int>(std::distance(result.begin(), mm.first));
                    std::cout << "Validation step " << step << " FAILED:" << std::endl;
                    std::cout << "  Expected: ";
                    for (int j = 0; j < 16; ++j) {
                        std::cout << expected[j] << " ";
                    }
                    std::cout << std::endl;
                    std::cout << "  Actual:   ";
                    for (int j = 0; j < 16; ++j) {
                        std::cout << result[j] << " ";
                    }
                    std::cout << std::endl;
                    std::cout << "  First mismatch at index " << idx << ": expected "
                              << expected[idx] << ", got " << result[idx] << std::endl;
                }
                all_passed = false;
            } else {
                std::cout << "Validation step " << step << " PASSED" << std::endl;
            }

            validation_index++;
        }
        
        step++;
        num_steps_processed++;
    }

    // Warn if input ended before completing all loaded validation steps
    if (has_validation && validation_index < validation_data.size() && reader.eof()) {
        std::cout << "WARNING: Reached end of input before completing all validation steps. Completed "
                  << validation_index << "/" << validation_data.size() << " validation steps." << std::endl;
    }
    
    // Report final results
    std::cout << std::endl << "Processing completed." << std::endl;
    std::cout << "Steps processed: " << num_steps_processed << std::endl;
    if (has_validation) {
        std::cout << "Validation results: " << (all_passed ? "PASS" : "FAIL") << std::endl;
        return all_passed ? 0 : 1;
    } else {
        std::cout << "Validation: Not performed" << std::endl;
        return 0;
    }
}

