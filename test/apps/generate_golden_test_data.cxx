/**
 * @file generate_golden_test_data.cxx
 *
 * @brief Utility to generate golden test data files for TPGenerator test application
 *
 * Generates:
 * - Input frames file (.bin) with 1-2 simple frames
 * - Validation file (.val) with expected TPs (or empty if no TPs expected)
 * - Config file (.json) for the test
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

 #include "tpglibs/testapp/common/BinaryFileValidator.hpp"
 #include "trgdataformats/TriggerPrimitive.hpp"
 #include <iostream>
 #include <fstream>
 #include <vector>
 #include <cstdint>
 #include <cstring>
 #include <algorithm>
 
 // ----------------------------------------------------------------------
 // Global constants
 // ----------------------------------------------------------------------
 
 // Geometry
 constexpr int NUM_CHANNELS = 64;
 constexpr int NUM_TIME_SAMPLES = 256;
 
 // Timing
 constexpr uint64_t FRAME_TIMESTAMP_MULTIPLIER = 1000;
 constexpr float SAMPLE_TICK_DIFFERENCE = 1.0f;
 
 // Per-plane thresholds (must match AVXThresholdProcessor config)
 constexpr int16_t PLANE0_THRESHOLD_ADC = 200;
 constexpr int16_t PLANE1_THRESHOLD_ADC = 300;
 constexpr int16_t PLANE2_THRESHOLD_ADC = 445;
 
 // Frame indices for this golden test
 constexpr int FRAME_INDEX_0 = 0;
 constexpr int FRAME_INDEX_1 = 1;
 constexpr int GOLDEN_NUM_FRAMES = 2;
 
 // Spike definition for the golden test
 // Single spike on channel 0, time samples 100-105 (inclusive), well above plane 0 threshold
 constexpr int SPIKE_CHANNEL = 0;
 constexpr int SPIKE_START_SAMPLE = 100;
 constexpr int SPIKE_END_SAMPLE = 105; // inclusive
 constexpr int SPIKE_SAMPLES_OVER_THRESHOLD =
   SPIKE_END_SAMPLE - SPIKE_START_SAMPLE + 1; // 6 samples
 
 // Amplitude: threshold + 100
 constexpr int16_t SPIKE_ADC_DELTA_ABOVE_THRESHOLD = 100;
 
 // TPGenerator detects the TP when processing t = 106 in this setup
 constexpr int SPIKE_DETECTION_SAMPLE = 106;
 
 // ----------------------------------------------------------------------
 // Helpers
 // ----------------------------------------------------------------------
 
 /**
  * @brief Channel → plane mapping used in config
  */
 inline int
 get_plane_for_channel(int ch)
 {
   return (ch < 16) ? 0 : ((ch < 32) ? 1 : ((ch < 48) ? 2 : 0));
 }
 
 /**
  * @brief Calculate absolute timestamp for TP time_start
  * Formula matches TPGenerator: (t - samples_over_threshold) * sample_tick_difference + timestamp
  * @param frame_index Frame index (for timestamp calculation)
  * @param time_sample Time sample index (t) where TP ends
  * @param samples_over_threshold Number of samples over threshold (affects time_start calculation)
  */
 uint64_t
 calculate_tp_time_start(int frame_index, int time_sample, int samples_over_threshold)
 {
   uint64_t frame_timestamp = static_cast<uint64_t>(frame_index) * FRAME_TIMESTAMP_MULTIPLIER;
   // Match TPGenerator formula: (t - samples_over_threshold) * sample_tick_difference + timestamp
   return static_cast<uint64_t>((time_sample - samples_over_threshold) * SAMPLE_TICK_DIFFERENCE) +
          frame_timestamp;
 }
 
 /**
  * @brief TP comparator for sorting (by time_start, channel, samples_over_threshold)
  */
 bool
 tp_less(const dunedaq::trgdataformats::TriggerPrimitive& a,
         const dunedaq::trgdataformats::TriggerPrimitive& b)
 {
   if (a.time_start != b.time_start)
     return a.time_start < b.time_start;
   if (a.channel != b.channel)
     return a.channel < b.channel;
   return a.samples_over_threshold < b.samples_over_threshold;
 }
 
 /**
  * @brief Write binary file header to stream
  */
 void
 write_file_header(std::ofstream& out)
 {
   tpglibs::testapp::BinaryFileHeader header;
   header.magic_number = tpglibs::testapp::BinaryFileValidator::MAGIC_NUMBER;
   header.version = tpglibs::testapp::BinaryFileValidator::VERSION;
   header.reserved = 0;
   out.write(reinterpret_cast<const char*>(&header), sizeof(header));
 }
 
 /**
  * @brief Write a frame to the frames file
  * @param out Output stream
  * @param frame_index Frame index (for timestamp)
  * @param adc_data ADC data: 64 channels × 256 time samples (row-major: time_sample[channel])
  */
 void
 write_frame(std::ofstream& out, int frame_index, const std::vector<int16_t>& adc_data)
 {
   // Frame header: 16 bytes (8 bytes timestamp + 8 bytes another_key)
   uint64_t timestamp = static_cast<uint64_t>(frame_index) * FRAME_TIMESTAMP_MULTIPLIER;
   uint64_t another_key = 0x123456789ABCDEF0; // Dummy value
 
   out.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
   out.write(reinterpret_cast<const char*>(&another_key), sizeof(another_key));
 
   // Frame data: NUM_CHANNELS × NUM_TIME_SAMPLES × 2 bytes
   // Layout: row-major (time_sample[channel])
   out.write(reinterpret_cast<const char*>(adc_data.data()),
             adc_data.size() * sizeof(int16_t));
 }
 
 /**
  * @brief Write TPs to validation file
  * @param out Output stream
  * @param frame_index Frame index
  * @param tps Vector of TPs to write
  */
 void
 write_tps(std::ofstream& out,
           uint32_t frame_index,
           const std::vector<dunedaq::trgdataformats::TriggerPrimitive>& tps)
 {
   uint32_t num_tps = static_cast<uint32_t>(tps.size());
 
   // Write frame index
   out.write(reinterpret_cast<const char*>(&frame_index), sizeof(frame_index));
 
   // Write TP count
   out.write(reinterpret_cast<const char*>(&num_tps), sizeof(num_tps));
 
   // Write TPs (sorted by time_start, channel, samples_over_threshold)
   if (num_tps > 0) {
     out.write(reinterpret_cast<const char*>(tps.data()),
               num_tps * sizeof(dunedaq::trgdataformats::TriggerPrimitive));
   }
 }
 
 /**
  * @brief Create a simple spike pattern in ADC data
  * Creates a spike in channel 0, time samples 100-105 (above plane 0 threshold)
  */
 void
 create_spike_pattern(std::vector<int16_t>& adc_data, int16_t threshold)
 {
   // Initialize all to zero (below threshold)
   std::fill(adc_data.begin(), adc_data.end(), 0);
 
   // Create a spike in channel 0, time samples 100-105
   // Layout: adc_data[time_sample * NUM_CHANNELS + channel]
   const int spike_value = static_cast<int>(threshold) + SPIKE_ADC_DELTA_ABOVE_THRESHOLD; // e.g. 300
 
   for (int t = SPIKE_START_SAMPLE; t <= SPIKE_END_SAMPLE; ++t) {
     adc_data[t * NUM_CHANNELS + SPIKE_CHANNEL] = static_cast<int16_t>(spike_value);
   }
 }
 
 int
 main(int argc, char* argv[])
 {
   if (argc != 2) {
     std::cerr << "Usage: " << argv[0] << " <output_directory>" << std::endl;
     std::cerr << "Example: " << argv[0] << " testdata/golden" << std::endl;
     return 1;
   }
 
   std::string output_dir = argv[1];
 
   // Output file paths
   std::string frames_file = output_dir + "/tpg_generator_golden_frames.bin";
   std::string validation_file = output_dir + "/tpg_generator_golden_val.val";
   std::string config_file = output_dir + "/tpg_generator_golden_config.json";
 
   // Threshold for plane 0
   constexpr int16_t PLANE0_THRESHOLD_FOR_TEST = PLANE0_THRESHOLD_ADC;
 
   std::cout << "Generating golden test data..." << std::endl;
   std::cout << "  Frames file: " << frames_file << std::endl;
   std::cout << "  Validation file: " << validation_file << std::endl;
   std::cout << "  Config file: " << config_file << std::endl;
 
   // Generate frames file
   {
     std::ofstream frames_out(frames_file, std::ios::binary);
     if (!frames_out) {
       std::cerr << "ERROR: Failed to open frames file: " << frames_file << std::endl;
       return 1;
     }
 
     write_file_header(frames_out);
 
     // Frame 0: Simple spike pattern (should produce TPs)
     std::vector<int16_t> frame0_data(NUM_CHANNELS * NUM_TIME_SAMPLES);
     create_spike_pattern(frame0_data, PLANE0_THRESHOLD_FOR_TEST);
     write_frame(frames_out, FRAME_INDEX_0, frame0_data);
 
     // Frame 1: All zeros (no TPs expected)
     std::vector<int16_t> frame1_data(NUM_CHANNELS * NUM_TIME_SAMPLES, 0);
     write_frame(frames_out, FRAME_INDEX_1, frame1_data);
 
     frames_out.close();
     std::cout << "  Generated frames file: " << GOLDEN_NUM_FRAMES << " frames" << std::endl;
   }
 
   // Generate validation file
   {
     std::ofstream val_out(validation_file, std::ios::binary);
     if (!val_out) {
       std::cerr << "ERROR: Failed to open validation file: " << validation_file << std::endl;
       return 1;
     }
 
     write_file_header(val_out);
 
     // Frame 0: Expected TPs from the spike pattern
     // Spike in channel 0, time samples 100-105 → 1 TP
     std::vector<dunedaq::trgdataformats::TriggerPrimitive> frame0_tps;
 
     dunedaq::trgdataformats::TriggerPrimitive tp{};
 
     // Set basic fields
     tp.channel = SPIKE_CHANNEL;                             // 0
     tp.samples_over_threshold = SPIKE_SAMPLES_OVER_THRESHOLD; // 6
     tp.adc_peak = static_cast<uint16_t>(PLANE0_THRESHOLD_FOR_TEST +
                                         SPIKE_ADC_DELTA_ABOVE_THRESHOLD); // 200 + 100 = 300
 
     // Calculate time_start using TPGenerator's formula:
     // (t - samples_over_threshold) * 1.0 + timestamp
     // For spike at samples 100-105, TPGenerator detects at t = 106
     tp.time_start = calculate_tp_time_start(FRAME_INDEX_0,
                                             SPIKE_DETECTION_SAMPLE,
                                             SPIKE_SAMPLES_OVER_THRESHOLD);
 
     // Calculate expected adc_integral: sum of ADC values above threshold
     // Spike value is (threshold + 100) = 300, for 6 samples
     const int spike_adc_value = static_cast<int>(PLANE0_THRESHOLD_FOR_TEST) +
                                 SPIKE_ADC_DELTA_ABOVE_THRESHOLD; // 300
     tp.adc_integral = static_cast<uint32_t>(spike_adc_value * SPIKE_SAMPLES_OVER_THRESHOLD);
     // 300 * 6 = 1800
 
     // Samples to peak: based on actual TPGenerator output, it's 0 for this case
     tp.samples_to_peak = 0;
 
     frame0_tps.push_back(tp);
 
     // Sorting is only needed if multiple TPs; kept for completeness
     if (frame0_tps.size() > 1) {
       std::sort(frame0_tps.begin(), frame0_tps.end(), tp_less);
     }
 
     write_tps(val_out, FRAME_INDEX_0, frame0_tps);
 
     // Frame 1: No TPs (empty)
     std::vector<dunedaq::trgdataformats::TriggerPrimitive> frame1_tps; // Empty
     write_tps(val_out, FRAME_INDEX_1, frame1_tps);
 
     val_out.close();
     std::cout << "  Generated validation file: " << GOLDEN_NUM_FRAMES
               << " frames (1 with TPs, 1 empty)" << std::endl;
   }
 
   // Generate config file
   {
     std::ofstream config_out(config_file);
     if (!config_out) {
       std::cerr << "ERROR: Failed to open config file: " << config_file << std::endl;
       return 1;
     }
 
     config_out << "{\n";
     config_out << "  \"processor_configs\": [\n";
     config_out << "    {\n";
     config_out << "      \"processor_name\": \"AVXThresholdProcessor\",\n";
     config_out << "      \"config\": {\n";
     config_out << "        \"plane0\": " << PLANE0_THRESHOLD_ADC << ",\n";
     config_out << "        \"plane1\": " << PLANE1_THRESHOLD_ADC << ",\n";
     config_out << "        \"plane2\": " << PLANE2_THRESHOLD_ADC << "\n";
     config_out << "      }\n";
     config_out << "    }\n";
     config_out << "  ],\n";
     config_out << "  \"test_config\": {\n";
     config_out << "    \"sample_tick_difference\": " << SAMPLE_TICK_DIFFERENCE << ",\n";
     config_out << "    \"sot_minima\": [1, 1, 1],\n";
     config_out << "    \"validation_frames\": [0, 1]\n";
     config_out << "  },\n";
     config_out << "  \"channel_plane_mappings\": [\n";
 
     // Generate 64 channel-plane mappings (0-15 -> plane 0, 16-31 -> plane 1, 32-47 -> plane 2, 48-63 -> plane 0)
     for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
       int plane = get_plane_for_channel(ch);
       config_out << "    [" << ch << ", " << plane << "]";
       if (ch < NUM_CHANNELS - 1)
         config_out << ",";
       config_out << "\n";
     }
 
     config_out << "  ]\n";
     config_out << "}\n";
 
     config_out.close();
     std::cout << "  Generated config file" << std::endl;
   }
 
   std::cout << "Golden test data generation complete!" << std::endl;
   return 0;
 }
 