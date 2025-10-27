/**
 * @file TPGProcessorTestApp.cpp
 *
 * @brief TPG Processor Test Application - Processes binary data through configured processors
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/testapp/reader/BinarySignalReader.hpp"
#include "tpglibs/AVXFactory.hpp"
#include "tpglibs/NaiveFactory.hpp"
#include "tpglibs/AbstractProcessor.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <cstring>
#include <algorithm>

struct InputFileHeader {
    uint32_t magic_number;
    uint32_t version;
    uint32_t data_type;
    uint32_t reserved;
};

struct ValidationFileHeader {
    uint32_t magic_number;
    uint32_t version;
    uint32_t num_steps;
    uint32_t reserved;
};

bool validate_input_header(std::ifstream& file) {
    InputFileHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(InputFileHeader));
    
    if (header.magic_number != 0x54414744) {  // "TAGD"
        std::cerr << "ERROR: Invalid input file magic number: 0x" 
                  << std::hex << header.magic_number << std::dec << std::endl;
        return false;
    }
    if (header.version != 0x010004) {  // 1.0.4 in hex
        std::cerr << "ERROR: Unsupported input file version: 0x" 
                  << std::hex << header.version << std::dec << std::endl;
        return false;
    }
    if (header.data_type != 0) {
        std::cerr << "ERROR: Unsupported data type: " << header.data_type << std::endl;
        return false;
    }
    return true;
}

bool validate_validation_header(std::ifstream& file) {
    ValidationFileHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(ValidationFileHeader));
    
    if (header.magic_number != 0x54414756) {  // "TAGV"
        std::cerr << "ERROR: Invalid validation file magic number: 0x" 
                  << std::hex << header.magic_number << std::dec << std::endl;
        return false;
    }
    if (header.version != 0x010004) {  // 1.0.4 in hex
        std::cerr << "ERROR: Unsupported validation file version: 0x" 
                  << std::hex << header.version << std::dec << std::endl;
        return false;
    }
    return true;
}

std::shared_ptr<tpglibs::AbstractProcessor<std::array<int16_t, 16>>> 
create_naive_processor(const std::string& processor_name) {
    auto factory = tpglibs::NaiveFactory::get_instance();
    auto processor = factory->create_processor(processor_name);
    if (!processor) {
        std::cerr << "ERROR: Failed to create processor: " << processor_name << std::endl;
        return nullptr;
    }
    return processor;
}

std::shared_ptr<tpglibs::AVXProcessor>
create_avx_processor(const std::string& processor_name) {
    auto factory = tpglibs::AVXFactory::get_instance();
    auto processor = factory->create_processor(processor_name);
    if (!processor) {
        std::cerr << "ERROR: Failed to create processor: " << processor_name << std::endl;
        return nullptr;
    }
    return processor;
}

std::array<int16_t, 16> convert_to_array(const std::vector<int16_t>& vec) {
    std::array<int16_t, 16> result;
    std::fill(result.begin(), result.end(), 0);
    std::copy(vec.begin(), vec.begin() + std::min(vec.size(), size_t(16)), result.begin());
    return result;
}

std::vector<int16_t> convert_to_vector(const std::array<int16_t, 16>& arr) {
    return std::vector<int16_t>(arr.begin(), arr.end());
}

__m256i array_to_avx(const std::array<int16_t, 16>& arr) {
    return _mm256_lddqu_si256(reinterpret_cast<const __m256i*>(arr.data()));
}

std::array<int16_t, 16> avx_to_array(const __m256i& avx_val) {
    std::array<int16_t, 16> result;
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(result.data()), avx_val);
    return result;
}

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name 
              << " <input_file> <processor_type> <config_file> <validation_file> <max_steps>" << std::endl;
    std::cerr << "Example: " << program_name 
              << " test_input.bin AVXRunSumProcessor config.json validation.val 20" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        print_usage(argv[0]);
        return 1;
    }
    
    std::string input_file = argv[1];
    std::string processor_type = argv[2];
    std::string config_file = argv[3];
    std::string validation_file = argv[4];
    int max_steps = std::atoi(argv[5]);
    
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
    
    if (!validate_input_header(input_stream)) {
        std::cerr << "ERROR: Invalid input file format" << std::endl;
        return 1;
    }
    
    // Validate validation file
    std::ifstream validation_stream(validation_file, std::ios::binary);
    if (!validation_stream.is_open()) {
        std::cerr << "ERROR: Failed to open validation file: " << validation_file << std::endl;
        return 1;
    }
    
    ValidationFileHeader val_header;
    if (!validate_validation_header(validation_stream)) {
        std::cerr << "ERROR: Invalid validation file format" << std::endl;
        return 1;
    }
    
    // Get validation steps from config
    auto validation_steps = config["test_config"]["validation_steps"];
    int samples_per_time_step = config["test_config"]["samples_per_time_step"];
    
    // Create processor based on type
    std::shared_ptr<tpglibs::AVXProcessor> avx_processor;
    std::shared_ptr<tpglibs::AbstractProcessor<std::array<int16_t, 16>>> naive_processor;
    bool is_avx = (processor_type.find("AVX") != std::string::npos);
    
    if (is_avx) {
        avx_processor = create_avx_processor(processor_type);
        if (!avx_processor) {
            return 1;
        }
    } else {
        naive_processor = create_naive_processor(processor_type);
        if (!naive_processor) {
            return 1;
        }
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
    // Skip the header (16 bytes)
    reader.seekg(16);
    
    // Read validation data (header was already read during validation, but we need the num_steps value)
    // So we rewind and read it again
    validation_stream.clear();
    validation_stream.seekg(0, std::ios::beg);
    validation_stream.read(reinterpret_cast<char*>(&val_header), sizeof(ValidationFileHeader));
    int num_validation_steps = val_header.num_steps;
    
    std::vector<std::vector<int16_t>> validation_data(num_validation_steps);
    for (int i = 0; i < num_validation_steps; ++i) {
        validation_data[i].resize(samples_per_time_step);
        validation_stream.read(reinterpret_cast<char*>(validation_data[i].data()), 
                             samples_per_time_step * sizeof(int16_t));
    }
    
    // Process data step by step
    int step = 0;
    int validation_index = 0;
    bool all_passed = true;
    int num_steps_processed = 0;
    
    std::cout << "Starting processing..." << std::endl;
    std::cout << "Processor: " << processor_type << std::endl;
    std::cout << "Max steps: " << max_steps << std::endl;
    std::cout << "Validation steps: ";
    for (auto step_num : validation_steps) {
        std::cout << step_num << " ";
    }
    std::cout << std::endl << std::endl;
    
    while (step < max_steps && !reader.eof()) {
        // Read one time step of data
        auto time_step_data = reader.next(samples_per_time_step);
        if (time_step_data.empty()) break;
        
        // Ensure we have exactly 16 samples
        while (time_step_data.size() < 16) {
            time_step_data.push_back(0);
        }
        
        // Convert to array
        std::array<int16_t, 16> input_array = convert_to_array(time_step_data);
        
        // Process the time step
        std::array<int16_t, 16> result;
        if (is_avx) {
            __m256i input_avx = array_to_avx(input_array);
            __m256i result_avx = avx_processor->process(input_avx);
            result = avx_to_array(result_avx);
        } else {
            result = naive_processor->process(input_array);
        }
        
        // Check if this is a validation step
        if (validation_index < static_cast<int>(validation_steps.size()) && step == static_cast<int>(validation_steps[validation_index])) {
            // Get expected result
            std::vector<int16_t> expected = validation_data[validation_index];
            
            // Compare actual vs expected
            bool step_passed = true;
            for (int i = 0; i < 16; ++i) {
                if (result[i] != expected[i]) {
                    step_passed = false;
                    if (!step_passed && all_passed) {
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
                        std::cout << "  First mismatch at index " << i << ": expected " 
                                  << expected[i] << ", got " << result[i] << std::endl;
                    }
                    break;
                }
            }
            
            if (!step_passed) {
                all_passed = false;
            } else {
                std::cout << "Validation step " << step << " PASSED" << std::endl;
            }
            
            validation_index++;
        }
        
        step++;
        num_steps_processed++;
    }
    
    // Report final results
    std::cout << std::endl << "Processing completed." << std::endl;
    std::cout << "Steps processed: " << num_steps_processed << std::endl;
    std::cout << "Validation results: " << (all_passed ? "PASS" : "FAIL") << std::endl;
    
    return all_passed ? 0 : 1;
}
