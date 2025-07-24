/**
 * @file processor_metric_collector_test.cxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE boost_test_macro_overview
#define FMT_HEADER_ONLY
#include <boost/test/unit_test.hpp>
#include "trgdataformats/Types.hpp"
#include "tpglibs/AVXProcessor.hpp"
#include <iostream>
#include "tpglibs/AVXFrugalPedestalSubtractProcessor.hpp"
#include "tpglibs/AVXPipeline.hpp"


#include "tpglibs/ProcessorMetricCollector.hpp"
#include <thread>
#include <chrono>
#include <atomic>

namespace tpglibs {
  
  BOOST_AUTO_TEST_CASE(test_processor_metric_collector_sanity) {
    // Sanity test: create, run, signal, get metrics, and stop
    std::vector<std::pair<std::string, nlohmann::json>> configs = {
      {
        "AVXFrugalPedestalSubtractProcessor",
        {
          {"accum_limit", 42}
        }
      },
      {
        "AVXFrugalPedestalSubtractProcessor",
        {
          {"accum_limit", 42}
        }
      },
    };

    // Lazy with the channel-plane assignments.
    std::vector<std::pair<dunedaq::trgdataformats::channel_t, int16_t>> channel_plane_numbers = 
    {{ 0, 0}, { 1, 0}, { 2, 0}, { 3, 0}, { 4, 0}, { 5, 0}, { 6, 0}, { 7, 0}, { 8, 0}, { 9, 0}, {10, 0}, {11, 0}, {12, 0}, {13, 0}, {14, 0}, {15, 0},
    {16, 1}, {17, 1}, {18, 1}, {19, 1}, {20, 1}, {21, 1}, {22, 1}, {23, 1}, {24, 1}, {25, 1}, {26, 1}, {27, 1}, {28, 1}, {29, 1}, {30, 1}, {31, 1},
    {32, 2}, {33, 2}, {34, 2}, {35, 2}, {36, 2}, {37, 2}, {38, 2}, {39, 2}, {40, 2}, {41, 2}, {42, 2}, {43, 2}, {44, 2}, {45, 2}, {46, 2}, {47, 2},
    {48, 0}, {49, 0}, {50, 0}, {51, 0}, {52, 0}, {53, 1}, {54, 1}, {55, 1}, {56, 1}, {57, 1}, {58, 2}, {59, 2}, {60, 2}, {61, 2}, {62, 2}, {63, 2}};
    ProcessorMetricCollector<__m256i> collector;

    collector.configure(configs, channel_plane_numbers, 4);

    std::string proc_name = "AVXFrugalPedestalSubtractProcessor";

    std::shared_ptr<AVXProcessor> pc = std::make_shared<AVXFrugalPedestalSubtractProcessor>();

    int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

    nlohmann::json pc_config = {
      {"accum_limit", 42}
    };

    pc->configure(pc_config, plane_numbers);

    collector.attach_processor(*pc.get(), proc_name, 1);

    auto processors = collector._get_attached_processors();

    BOOST_TEST(processors[0] != nullptr);
    BOOST_TEST(processors[1] == nullptr);

    auto info = collector._get_processor_metric_table();

    BOOST_TEST(info[0].m_pipeline_id == 1);
    BOOST_TEST(info[0].m_names_of_metrics.size() == 2);

    // simulate a metric store in processor
    pc->save_metric_to_store_buffer();

    // Launch run loop in background
    collector.run();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Trigger some signal collects
    collector.signal_collect();

    collector.signal_collect();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Stop collector and join thread
    collector.stop();

    auto metrics = collector.get_retrieved_processor_metrics();

    BOOST_TEST(metrics.size() == 1); // collecting from one processor
    BOOST_TEST(metrics[0].size() == 2); // collecting m_accum and m_pedestal

    int16_t out0[16], out1[16];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out0), metrics[0][0]);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out1), metrics[0][1]);

    for (size_t i  = 0; i < 16; i++) {
      BOOST_TEST(out0[i] == 16384);
      BOOST_TEST(out1[i] == 0);
    }

    auto casted_table = collector._get_processor_casted_data_table();

    for (size_t i  = 0; i < 16; i++) {
      BOOST_TEST(casted_table[0][0][i] == 16384);
      BOOST_TEST(casted_table[0][1][i] == 0);
    }

  }

  BOOST_AUTO_TEST_CASE(test_processor_metric_collector_pipeline_test) {
    // Sanity test: create, run, signal, get metrics, and stop
    std::vector<std::pair<std::string, nlohmann::json>> configs = {
      {
        "AVXFrugalPedestalSubtractProcessor",
        {
          {"accum_limit", 42},
          {"metric_collect_data_sample_rate", 1024}
        }
      },
      {
        "AVXFrugalPedestalSubtractProcessor",
        {
          {"accum_limit", 42},
          {"metric_collect_data_sample_rate", 1024}
        }
      },
    };

    // Lazy with the channel-plane assignments.
    std::vector<std::pair<dunedaq::trgdataformats::channel_t, int16_t>>
    channel_plane_numbers = {{  0, 0},
                             { 10, 0},
                             { 20, 0},
                             { 30, 0},
                             { 40, 0},
                             {100, 1},
                             {110, 1},
                             {120, 1},
                             {130, 1},
                             {140, 1},
                             {200, 2},
                             {210, 2},
                             {220, 2},
                             {230, 2},
                             {240, 2},
                             {250, 2}};

    ProcessorMetricCollector<__m256i> collector;

    AVXPipeline pipeline = AVXPipeline();
    std::vector<uint16_t> sot_minima = {1,1,1};

    collector.configure(configs, channel_plane_numbers, 1);

    pipeline.configure(configs, channel_plane_numbers);
    pipeline.set_sot_minima(sot_minima);

    pipeline.attach_to_metric_collector(collector, 1);

    auto attached_processors = collector._get_attached_processors();
    
    BOOST_TEST(attached_processors.size() == 2);
    BOOST_TEST(attached_processors[0] != nullptr);
    BOOST_TEST(attached_processors[1] != nullptr);

    auto info = collector._get_processor_metric_table();

    BOOST_TEST(info[0].m_names_of_metrics[0] == "m_pedestal");
    BOOST_TEST(info[0].m_names_of_metrics[1] == "m_accum");
    BOOST_TEST(info[0].m_pipeline_id == 1);

    BOOST_TEST(info[1].m_names_of_metrics[0] == "m_pedestal");
    BOOST_TEST(info[1].m_names_of_metrics[1] == "m_accum");
    BOOST_TEST(info[1].m_pipeline_id == 1);

    __m256i input = _mm256_set1_epi16(0x4000);
    // Control flag to stop the pipeline thread
    std::atomic<bool> run_pipeline(true);

    // Launch pipeline processing in a separate thread
    std::thread pipeline_thread([&pipeline, &run_pipeline, input]() {
      while (run_pipeline.load()) {
        pipeline.process(input);
      }
    });

    collector.run();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Trigger metric collect

    collector.signal_collect();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Stop collector and join thread
    collector.stop();
    // Signal the pipeline thread to stop and wait for it to finish
    run_pipeline.store(false);
    pipeline_thread.join();

    // test for values

    auto metrics = collector.get_retrieved_processor_metrics();

    BOOST_TEST(metrics.size() == 2); // collecting from one processor
    BOOST_TEST(metrics[0].size() == 2); // collecting m_accum and m_pedestal
    BOOST_TEST(metrics[1].size() == 2); // collecting m_accum and m_pedestal

    int16_t out0[16], out1[16];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out0), metrics[0][0]);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out1), metrics[0][1]);

    for (size_t i  = 0; i < 16; i++) {
      BOOST_TEST(out0[i] == 16384);
      BOOST_TEST(out1[i] == 0);
    }

    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out0), metrics[1][0]);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out1), metrics[1][1]);

    for (size_t i  = 0; i < 16; i++) {
      // the second processor sees pedestal 0
      BOOST_TEST(out0[i] == 0);
      BOOST_TEST(out1[i] == 0);
    }

    auto casted_table = collector._get_processor_casted_data_table();

    for (size_t i  = 0; i < 16; i++) {
      BOOST_TEST(casted_table[0][0][i] == 16384);
      BOOST_TEST(casted_table[0][1][i] == 0);
    }

    for (size_t i  = 0; i < 16; i++) {
      // the second processor sees pedestal 0
      BOOST_TEST(casted_table[1][0][i] == 0);
      BOOST_TEST(casted_table[1][1][i] == 0);
    }

  }

  BOOST_AUTO_TEST_CASE(test_processor_metric_collector_pipeline_test_no_process) {
    // Sanity test: create, run, signal, get metrics, and stop
    std::vector<std::pair<std::string, nlohmann::json>> configs = {
      {
        "AVXFrugalPedestalSubtractProcessor",
        {
          {"accum_limit", 42},
          {"metric_collect_data_sample_rate", 1024}
        }
      },
      {
        "AVXFrugalPedestalSubtractProcessor",
        {
          {"accum_limit", 42},
          {"metric_collect_data_sample_rate", 1024}
        }
      },
    };

    // Lazy with the channel-plane assignments.
    std::vector<std::pair<dunedaq::trgdataformats::channel_t, int16_t>>
    channel_plane_numbers = {{  0, 0},
                             { 10, 0},
                             { 20, 0},
                             { 30, 0},
                             { 40, 0},
                             {100, 1},
                             {110, 1},
                             {120, 1},
                             {130, 1},
                             {140, 1},
                             {200, 2},
                             {210, 2},
                             {220, 2},
                             {230, 2},
                             {240, 2},
                             {250, 2}};

    ProcessorMetricCollector<__m256i> collector;

    AVXPipeline pipeline = AVXPipeline();
    std::vector<uint16_t> sot_minima = {1,1,1};

    collector.configure(configs, channel_plane_numbers, 1);

    pipeline.configure(configs, channel_plane_numbers);
    pipeline.set_sot_minima(sot_minima);

    pipeline.attach_to_metric_collector(collector, 1);

    auto attached_processors = collector._get_attached_processors();
    
    BOOST_TEST(attached_processors.size() == 2);
    BOOST_TEST(attached_processors[0] != nullptr);
    BOOST_TEST(attached_processors[1] != nullptr);

    auto info = collector._get_processor_metric_table();

    BOOST_TEST(info[0].m_names_of_metrics[0] == "m_pedestal");
    BOOST_TEST(info[0].m_names_of_metrics[1] == "m_accum");
    BOOST_TEST(info[0].m_pipeline_id == 1);

    BOOST_TEST(info[1].m_names_of_metrics[0] == "m_pedestal");
    BOOST_TEST(info[1].m_names_of_metrics[1] == "m_accum");
    BOOST_TEST(info[1].m_pipeline_id == 1);

    __m256i input = _mm256_set1_epi16(0x4000);
    // Launch pipeline processing in a separate thread
    for (auto & proc: attached_processors) {
      proc->save_metric_to_store_buffer();
    }

    collector.run();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Trigger metric collect

    collector.signal_collect();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Stop collector and join thread
    collector.stop();
    // test for values

    auto metrics = collector.get_retrieved_processor_metrics();

    BOOST_TEST(metrics.size() == 2); // collecting from one processor
    BOOST_TEST(metrics[0].size() == 2); // collecting m_accum and m_pedestal
    BOOST_TEST(metrics[1].size() == 2); // collecting m_accum and m_pedestal

    int16_t out0[16], out1[16];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out0), metrics[0][0]);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out1), metrics[0][1]);

    for (size_t i  = 0; i < 16; i++) {
      BOOST_TEST(out0[i] == 16384);
      BOOST_TEST(out1[i] == 0);
    }

    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out0), metrics[1][0]);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out1), metrics[1][1]);

    for (size_t i  = 0; i < 16; i++) {
      BOOST_TEST(out0[i] == 16384);
      BOOST_TEST(out1[i] == 0);
    }

    auto casted_table = collector._get_processor_casted_data_table();

    for (size_t i  = 0; i < 16; i++) {
      BOOST_TEST(casted_table[0][0][i] == 16384);
      BOOST_TEST(casted_table[0][1][i] == 0);
    }

    for (size_t i  = 0; i < 16; i++) {
      BOOST_TEST(casted_table[1][0][i] == 16384);
      BOOST_TEST(casted_table[1][1][i] == 0);
    }

  }

} // tpglibs
