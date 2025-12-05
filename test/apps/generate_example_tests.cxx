/**
 * @file generate_example_tests.cxx
 *
 * @brief Utility to generate example test files for TPGenerator test application
 *
 * Generates multiple example test cases:
 * - Simple sanity test (constant input, known TP output)
 * - Multi-frame test (multiple frames, validation at specific steps)
 * - Multi-pipeline test (4 pipelines, 64 channels)
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
 #include <filesystem>
 
 // ----------------------------------------------------------------------
 // Global constants for test geometry & timing
 // ----------------------------------------------------------------------
 
 // Geometry
 constexpr int NUM_CHANNELS = 64;
 constexpr int NUM_TIME_SAMPLES = 256;
 
 // Timing
 constexpr uint64_t FRAME_TIMESTAMP_MULTIPLIER = 1000;
 constexpr float SAMPLE_TICK_DIFFERENCE = 1.0f;
 
 // Per-plane thresholds used across all tests
 constexpr int16_t PLANE0_THRESHOLD_ADC = 200;
 constexpr int16_t PLANE1_THRESHOLD_ADC = 300;
 constexpr int16_t PLANE2_THRESHOLD_ADC = 445;
 
 // Channel-to-plane mapping helper
 inline int
 get_plane_for_channel(int ch)
 {
   return (ch < 16) ? 0 : ((ch < 32) ? 1 : ((ch < 48) ? 2 : 0));
 }
 
 // ----------------------------------------------------------------------
 // Multi-frame test layout & spike definitions
 // ----------------------------------------------------------------------
 
 // Frame indices
 constexpr int FRAME_INDEX_0 = 0;
 constexpr int FRAME_INDEX_1 = 1;
 constexpr int FRAME_INDEX_2 = 2;
 constexpr int FRAME_INDEX_3 = 3;
 constexpr int FRAME_INDEX_4 = 4;
 constexpr int MULTIFRAME_NUM_FRAMES = 5;
 
 // Frame 0: single spike on channel 0 (plane 0)
 constexpr int FRAME0_CH0_SPIKE_CHANNEL = 0;
 constexpr int FRAME0_CH0_SPIKE_START = 100;
 constexpr int FRAME0_CH0_SPIKE_END = 105; // inclusive
 constexpr int16_t FRAME0_CH0_SPIKE_ADC_DELTA = 100; // over PLANE0_THRESHOLD_ADC
 constexpr int FRAME0_CH0_SPIKE_SAMPLES =
   FRAME0_CH0_SPIKE_END - FRAME0_CH0_SPIKE_START + 1; // 6
 
 // Frame 2: single spike on channel 16 (plane 1)
 constexpr int FRAME2_CH16_SPIKE_CHANNEL = 16;
 constexpr int FRAME2_CH16_SPIKE_START = 150;
 constexpr int FRAME2_CH16_SPIKE_END = 155; // inclusive
 constexpr int16_t FRAME2_CH16_SPIKE_ADC_DELTA = 150; // over PLANE0_THRESHOLD_ADC in original code
 constexpr int FRAME2_CH16_SPIKE_SAMPLES =
   FRAME2_CH16_SPIKE_END - FRAME2_CH16_SPIKE_START + 1; // 6
 
 // Frame 4: multi-channel spikes
 // Channel 0 (plane 0, threshold 200): spike at time 50-55
 constexpr int FRAME4_CH0_SPIKE_CHANNEL = 0;
 constexpr int FRAME4_CH0_SPIKE_START = 50;
 constexpr int FRAME4_CH0_SPIKE_END = 55; // inclusive
 constexpr int16_t FRAME4_CH0_SPIKE_ADC_DELTA = 50; // over PLANE0_THRESHOLD_ADC
 constexpr int FRAME4_CH0_SPIKE_SAMPLES =
   FRAME4_CH0_SPIKE_END - FRAME4_CH0_SPIKE_START + 1; // 6
 
 // Channel 16 (plane 1, threshold 300): spike at time 100-108
 constexpr int FRAME4_CH16_SPIKE_CHANNEL = 16;
 constexpr int FRAME4_CH16_SPIKE_START = 100;
 constexpr int FRAME4_CH16_SPIKE_END = 108; // inclusive
 constexpr int16_t FRAME4_CH16_SPIKE_ADC_DELTA = 100; // over PLANE1_THRESHOLD_ADC
 constexpr int FRAME4_CH16_SPIKE_SAMPLES =
   FRAME4_CH16_SPIKE_END - FRAME4_CH16_SPIKE_START + 1; // 9
 
 // Channel 32 (plane 2, threshold 445): spike at time 150-152
 constexpr int FRAME4_CH32_SPIKE_CHANNEL = 32;
 constexpr int FRAME4_CH32_SPIKE_START = 150;
 constexpr int FRAME4_CH32_SPIKE_END = 152; // inclusive
 constexpr int16_t FRAME4_CH32_SPIKE_ADC_DELTA = 100; // over PLANE2_THRESHOLD_ADC
 constexpr int FRAME4_CH32_SPIKE_SAMPLES =
   FRAME4_CH32_SPIKE_END - FRAME4_CH32_SPIKE_START + 1; // 3
 
 // Channel 48 (plane 0, threshold 200): spike at time 200-205
 constexpr int FRAME4_CH48_SPIKE_CHANNEL = 48;
 constexpr int FRAME4_CH48_SPIKE_START = 200;
 constexpr int FRAME4_CH48_SPIKE_END = 205; // inclusive
 constexpr int16_t FRAME4_CH48_SPIKE_ADC_DELTA = 120; // over PLANE0_THRESHOLD_ADC
 constexpr int FRAME4_CH48_SPIKE_SAMPLES =
   FRAME4_CH48_SPIKE_END - FRAME4_CH48_SPIKE_START + 1; // 6
 
 // ----------------------------------------------------------------------
 // Utility functions
 // ----------------------------------------------------------------------
 
 /**
  * @brief Calculate absolute timestamp for TP time_start
  * Formula: frame_timestamp + (time_sample * sample_tick_difference)
  */
 uint64_t
 calculate_tp_time_start(int frame_index, int time_sample)
 {
   uint64_t frame_timestamp = static_cast<uint64_t>(frame_index) * FRAME_TIMESTAMP_MULTIPLIER;
   return frame_timestamp + static_cast<uint64_t>(time_sample * SAMPLE_TICK_DIFFERENCE);
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
  */
 void
 write_frame(std::ofstream& out, int frame_index, const std::vector<int16_t>& adc_data)
 {
   uint64_t timestamp = static_cast<uint64_t>(frame_index) * FRAME_TIMESTAMP_MULTIPLIER;
   uint64_t another_key = 0x123456789ABCDEF0;
 
   out.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
   out.write(reinterpret_cast<const char*>(&another_key), sizeof(another_key));
   out.write(reinterpret_cast<const char*>(adc_data.data()), adc_data.size() * sizeof(int16_t));
 }
 
 /**
  * @brief Write TPs to validation file
  */
 void
 write_tps(std::ofstream& out,
           uint32_t frame_index,
           const std::vector<dunedaq::trgdataformats::TriggerPrimitive>& tps)
 {
   uint32_t num_tps = static_cast<uint32_t>(tps.size());
   out.write(reinterpret_cast<const char*>(&frame_index), sizeof(frame_index));
   out.write(reinterpret_cast<const char*>(&num_tps), sizeof(num_tps));
   if (num_tps > 0) {
     out.write(reinterpret_cast<const char*>(tps.data()),
               num_tps * sizeof(dunedaq::trgdataformats::TriggerPrimitive));
   }
 }
 
 /**
  * @brief Create constant ADC pattern (all same value)
  */
 void
 create_constant_pattern(std::vector<int16_t>& adc_data, int16_t value)
 {
   std::fill(adc_data.begin(), adc_data.end(), value);
 }
 
 /**
  * @brief Create spike pattern in specific channel and time range
  */
 void
 create_spike_pattern(std::vector<int16_t>& adc_data,
                      int channel,
                      int time_start,
                      int time_end,
                      int16_t spike_value)
 {
   for (int t = time_start; t <= time_end; ++t) {
     adc_data[t * NUM_CHANNELS + channel] = spike_value;
   }
 }
 
 /**
  * @brief Create multi-channel spike pattern (multiple channels with spikes)
  */
 void
 create_multi_channel_spikes(std::vector<int16_t>& adc_data)
 {
   std::fill(adc_data.begin(), adc_data.end(), 0);
 
   // Channel 0 (plane 0, threshold 200): spike at time 50-55
   create_spike_pattern(adc_data,
                        FRAME4_CH0_SPIKE_CHANNEL,
                        FRAME4_CH0_SPIKE_START,
                        FRAME4_CH0_SPIKE_END,
                        static_cast<int16_t>(PLANE0_THRESHOLD_ADC + FRAME4_CH0_SPIKE_ADC_DELTA));
 
   // Channel 16 (plane 1, threshold 300): spike at time 100-108
   create_spike_pattern(adc_data,
                        FRAME4_CH16_SPIKE_CHANNEL,
                        FRAME4_CH16_SPIKE_START,
                        FRAME4_CH16_SPIKE_END,
                        static_cast<int16_t>(PLANE1_THRESHOLD_ADC + FRAME4_CH16_SPIKE_ADC_DELTA));
 
   // Channel 32 (plane 2, threshold 445): spike at time 150-152
   create_spike_pattern(adc_data,
                        FRAME4_CH32_SPIKE_CHANNEL,
                        FRAME4_CH32_SPIKE_START,
                        FRAME4_CH32_SPIKE_END,
                        static_cast<int16_t>(PLANE2_THRESHOLD_ADC + FRAME4_CH32_SPIKE_ADC_DELTA));
 
   // Channel 48 (plane 0, threshold 200): spike at time 200-205
   create_spike_pattern(adc_data,
                        FRAME4_CH48_SPIKE_CHANNEL,
                        FRAME4_CH48_SPIKE_START,
                        FRAME4_CH48_SPIKE_END,
                        static_cast<int16_t>(PLANE0_THRESHOLD_ADC + FRAME4_CH48_SPIKE_ADC_DELTA));
 }
 
 // ----------------------------------------------------------------------
 // Test generators
 // ----------------------------------------------------------------------
 
 /**
  * @brief Generate simple sanity test (constant input)
  */
 void
 generate_sanity_test(const std::string& output_dir)
 {
   std::string frames_file = output_dir + "/tpg_generator_sanity_frames.bin";
   std::string validation_file = output_dir + "/tpg_generator_sanity_val.val";
   std::string config_file = output_dir + "/tpg_generator_sanity_config.json";
 
   std::cout << "Generating sanity test..." << std::endl;
 
   // Generate frames: all zeros (below threshold, no TPs)
   {
     std::ofstream frames_out(frames_file, std::ios::binary);
     write_file_header(frames_out);
 
     std::vector<int16_t> frame_data(NUM_CHANNELS * NUM_TIME_SAMPLES, 0);
     write_frame(frames_out, FRAME_INDEX_0, frame_data);
 
     frames_out.close();
   }
 
   // Generate validation: no TPs expected
   {
     std::ofstream val_out(validation_file, std::ios::binary);
     write_file_header(val_out);
 
     std::vector<dunedaq::trgdataformats::TriggerPrimitive> empty_tps;
     write_tps(val_out, FRAME_INDEX_0, empty_tps);
 
     val_out.close();
   }
 
   // Generate config
   {
     std::ofstream config_out(config_file);
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
     config_out << "    \"validation_frames\": [0]\n";
     config_out << "  },\n";
     config_out << "  \"channel_plane_mappings\": [\n";
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
   }
 
   std::cout << "  Generated: " << frames_file << std::endl;
   std::cout << "  Generated: " << validation_file << std::endl;
   std::cout << "  Generated: " << config_file << std::endl;
 }
 
 /**
  * @brief Generate multi-frame test
  */
 void
 generate_multiframe_test(const std::string& output_dir)
 {
   std::string frames_file = output_dir + "/tpg_generator_multiframe_frames.bin";
   std::string validation_file = output_dir + "/tpg_generator_multiframe_val.val";
   std::string config_file = output_dir + "/tpg_generator_multiframe_config.json";
 
   std::cout << "Generating multi-frame test..." << std::endl;
 
   // Generate frames: 5 frames with different patterns
   {
     std::ofstream frames_out(frames_file, std::ios::binary);
     write_file_header(frames_out);
 
     for (int frame_idx = 0; frame_idx < MULTIFRAME_NUM_FRAMES; ++frame_idx) {
       std::vector<int16_t> frame_data(NUM_CHANNELS * NUM_TIME_SAMPLES, 0);
 
       if (frame_idx == FRAME_INDEX_0) {
         // Frame 0: spike in channel 0
         create_spike_pattern(frame_data,
                              FRAME0_CH0_SPIKE_CHANNEL,
                              FRAME0_CH0_SPIKE_START,
                              FRAME0_CH0_SPIKE_END,
                              static_cast<int16_t>(PLANE0_THRESHOLD_ADC + FRAME0_CH0_SPIKE_ADC_DELTA));
       } else if (frame_idx == FRAME_INDEX_2) {
         // Frame 2: spike in channel 16
         create_spike_pattern(frame_data,
                              FRAME2_CH16_SPIKE_CHANNEL,
                              FRAME2_CH16_SPIKE_START,
                              FRAME2_CH16_SPIKE_END,
                              static_cast<int16_t>(PLANE0_THRESHOLD_ADC + FRAME2_CH16_SPIKE_ADC_DELTA));
       } else if (frame_idx == FRAME_INDEX_4) {
         // Frame 4: multi-channel spikes
         create_multi_channel_spikes(frame_data);
       }
       // Frames 1 and 3: all zeros
 
       write_frame(frames_out, frame_idx, frame_data);
     }
 
     frames_out.close();
   }
 
   // Generate validation: TPs for frames 0, 2, 4
   {
     std::ofstream val_out(validation_file, std::ios::binary);
     write_file_header(val_out);
 
     // Frame 0: 1 TP
     {
       dunedaq::trgdataformats::TriggerPrimitive tp{};
       tp.time_start = calculate_tp_time_start(FRAME_INDEX_0, FRAME0_CH0_SPIKE_START);
       tp.channel = FRAME0_CH0_SPIKE_CHANNEL;
       tp.adc_peak = static_cast<uint16_t>(PLANE0_THRESHOLD_ADC + FRAME0_CH0_SPIKE_ADC_DELTA);
       tp.samples_over_threshold = FRAME0_CH0_SPIKE_SAMPLES;
       tp.adc_integral = static_cast<uint32_t>((PLANE0_THRESHOLD_ADC + FRAME0_CH0_SPIKE_ADC_DELTA) *
                                               FRAME0_CH0_SPIKE_SAMPLES);
       tp.samples_to_peak = 0;
 
       write_tps(val_out, FRAME_INDEX_0, { tp });
     }
 
     // Frame 1: no TPs
     write_tps(val_out, FRAME_INDEX_1, {});
 
     // Frame 2: 1 TP
     {
       dunedaq::trgdataformats::TriggerPrimitive tp{};
       tp.time_start = calculate_tp_time_start(FRAME_INDEX_2, FRAME2_CH16_SPIKE_START);
       tp.channel = FRAME2_CH16_SPIKE_CHANNEL;
       tp.adc_peak = static_cast<uint16_t>(PLANE0_THRESHOLD_ADC + FRAME2_CH16_SPIKE_ADC_DELTA);
       tp.samples_over_threshold = FRAME2_CH16_SPIKE_SAMPLES;
       tp.adc_integral = static_cast<uint32_t>((PLANE0_THRESHOLD_ADC + FRAME2_CH16_SPIKE_ADC_DELTA) *
                                               FRAME2_CH16_SPIKE_SAMPLES);
       tp.samples_to_peak = 0;
 
       write_tps(val_out, FRAME_INDEX_2, { tp });
     }
 
     // Frame 3: no TPs
     write_tps(val_out, FRAME_INDEX_3, {});
 
     // Frame 4: multiple TPs (from multi-channel spikes)
     {
       std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps;
 
       // Channel 0: time 50-55
       {
         dunedaq::trgdataformats::TriggerPrimitive tp{};
         tp.time_start = calculate_tp_time_start(FRAME_INDEX_4, FRAME4_CH0_SPIKE_START);
         tp.channel = FRAME4_CH0_SPIKE_CHANNEL;
         tp.adc_peak = static_cast<uint16_t>(PLANE0_THRESHOLD_ADC + FRAME4_CH0_SPIKE_ADC_DELTA);
         tp.samples_over_threshold = FRAME4_CH0_SPIKE_SAMPLES;
         tp.adc_integral = static_cast<uint32_t>((PLANE0_THRESHOLD_ADC + FRAME4_CH0_SPIKE_ADC_DELTA) *
                                                 FRAME4_CH0_SPIKE_SAMPLES);
         tp.samples_to_peak = 0;
         tps.push_back(tp);
       }
 
       // Channel 16: time 100-108 (plane 1, threshold 300, spike value 400)
       {
         dunedaq::trgdataformats::TriggerPrimitive tp{};
         tp.time_start = calculate_tp_time_start(FRAME_INDEX_4, FRAME4_CH16_SPIKE_START);
         tp.channel = FRAME4_CH16_SPIKE_CHANNEL;
         tp.adc_peak = static_cast<uint16_t>(PLANE1_THRESHOLD_ADC + FRAME4_CH16_SPIKE_ADC_DELTA);
         tp.samples_over_threshold = FRAME4_CH16_SPIKE_SAMPLES;
         tp.adc_integral = static_cast<uint32_t>((PLANE1_THRESHOLD_ADC + FRAME4_CH16_SPIKE_ADC_DELTA) *
                                                 FRAME4_CH16_SPIKE_SAMPLES);
         tp.samples_to_peak = 0;
         tps.push_back(tp);
       }
 
       // Channel 32: time 150-152 (plane 2, threshold 445, spike value 545)
       {
         dunedaq::trgdataformats::TriggerPrimitive tp{};
         tp.time_start = calculate_tp_time_start(FRAME_INDEX_4, FRAME4_CH32_SPIKE_START);
         tp.channel = FRAME4_CH32_SPIKE_CHANNEL;
         tp.adc_peak = static_cast<uint16_t>(PLANE2_THRESHOLD_ADC + FRAME4_CH32_SPIKE_ADC_DELTA);
         tp.samples_over_threshold = FRAME4_CH32_SPIKE_SAMPLES;
         tp.adc_integral = static_cast<uint32_t>((PLANE2_THRESHOLD_ADC + FRAME4_CH32_SPIKE_ADC_DELTA) *
                                                 FRAME4_CH32_SPIKE_SAMPLES);
         tp.samples_to_peak = 0;
         tps.push_back(tp);
       }
 
       // Channel 48: time 200-205
       {
         dunedaq::trgdataformats::TriggerPrimitive tp{};
         tp.time_start = calculate_tp_time_start(FRAME_INDEX_4, FRAME4_CH48_SPIKE_START);
         tp.channel = FRAME4_CH48_SPIKE_CHANNEL;
         tp.adc_peak = static_cast<uint16_t>(PLANE0_THRESHOLD_ADC + FRAME4_CH48_SPIKE_ADC_DELTA);
         tp.samples_over_threshold = FRAME4_CH48_SPIKE_SAMPLES;
         tp.adc_integral = static_cast<uint32_t>((PLANE0_THRESHOLD_ADC + FRAME4_CH48_SPIKE_ADC_DELTA) *
                                                 FRAME4_CH48_SPIKE_SAMPLES);
         tp.samples_to_peak = 0;
         tps.push_back(tp);
       }
 
       std::sort(tps.begin(), tps.end(), tp_less);
       write_tps(val_out, FRAME_INDEX_4, tps);
     }
 
     val_out.close();
   }
 
   // Generate config
   {
     std::ofstream config_out(config_file);
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
     config_out << "    \"validation_frames\": [0, 1, 2, 3, 4],\n";
     config_out << "    \"max_frames\": " << MULTIFRAME_NUM_FRAMES << "\n";
     config_out << "  },\n";
     config_out << "  \"channel_plane_mappings\": [\n";
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
   }
 
   std::cout << "  Generated: " << frames_file << std::endl;
   std::cout << "  Generated: " << validation_file << std::endl;
   std::cout << "  Generated: " << config_file << std::endl;
 }
 
 int
 main(int argc, char* argv[])
 {
   if (argc != 2) {
     std::cerr << "Usage: " << argv[0] << " <output_directory>" << std::endl;
     std::cerr << "Example: " << argv[0] << " testdata/examples" << std::endl;
     return 1;
   }
 
   std::string output_dir = argv[1];
 
   // Create output directory if it doesn't exist
   try {
     std::filesystem::create_directories(output_dir);
   } catch (const std::exception& e) {
     std::cerr << "ERROR: Failed to create output directory: " << e.what() << std::endl;
     return 1;
   }
 
   std::cout << "Generating example test files in: " << output_dir << std::endl;
   std::cout << std::endl;
 
   generate_sanity_test(output_dir);
   std::cout << std::endl;
   generate_multiframe_test(output_dir);
 
   std::cout << std::endl;
   std::cout << "Example test generation complete!" << std::endl;
   std::cout << "Note: TP values in validation files are estimates." << std::endl;
   std::cout << "Run the test app to verify actual TP values match." << std::endl;
 
   return 0;
 }
 