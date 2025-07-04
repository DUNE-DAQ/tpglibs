/**
 * @file avx_pipeline_test.cxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE boost_test_macro_overview
#define FMT_HEADER_ONLY

#include "tpglibs/AVXPipeline.hpp"

#include "trgdataformats/Types.hpp"

#include <boost/test/unit_test.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>

#include <iostream>

namespace tpglibs {

BOOST_AUTO_TEST_CASE(test_macro_overview)
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
  std::vector<__m256i> signals = {_mm256_set1_epi16(100),
                                  _mm256_set1_epi16(200),
                                  _mm256_set1_epi16(1000),
                                  _mm256_set1_epi16(200),
                                  _mm256_set1_epi16(100),
                                  _mm256_set1_epi16(0),
                                  _mm256_set1_epi16(-100),
                                  _mm256_set1_epi16(-200),
                                  _mm256_set1_epi16(-1000),
                                  _mm256_set1_epi16(-200),
                                  _mm256_set1_epi16(-100),
                                  _mm256_set1_epi16(0)};
#pragma GCC diagnostic pop

  AVXPipeline pipeline = AVXPipeline();

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

  // Horrendous brackets.
  std::vector<std::pair<std::string, nlohmann::json>> configs = {
    {
      "AVXRunSumProcessor",
      {
        {"memory_factor_plane0", 10},
        {"memory_factor_plane1", 10},
        {"memory_factor_plane2", 10},
        {"scale_factor_plane0", 10},
        {"scale_factor_plane1", 10},
        {"scale_factor_plane2", 10},
      }
    },
    {
      "AVXThresholdProcessor",
      {
        {"plane0", 200},
        {"plane1", 1000},
        {"plane2", 1500}
      }
    }
  };

  std::vector<uint16_t> sot_minima = {1,1,1};

  pipeline.configure(configs, channel_plane_numbers);
  pipeline.set_sot_minima(sot_minima);

  // ADC peak should max at 1600 for all channels.
  bool adc_peak_at_1600 = true;

  for (const __m256i& signal : signals) {
    std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps = pipeline.process(signal);
    if (tps.empty()) continue;

    for (auto tp : tps) {
      if (tp.adc_peak != 1600) {
        adc_peak_at_1600 = false;
        break;
      }
    }
  }

  // ---------------------------
  // Test for metric collection at AVXPipeline

  std::vector<std::pair<std::string, nlohmann::json>> configs2 = {
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
    {
      "AVXFrugalPedestalSubtractProcessor",
      {
        {"accum_limit", 42}
      }
    }
  };

  AVXPipeline pipeline2 = AVXPipeline();

  pipeline2.configure(configs2, channel_plane_numbers);
  pipeline2.set_sot_minima(sot_minima);

  std::unordered_map<tpglibs::MetricBufferKey, tpglibs::IndexAwareSignalPointer<__m256i>> table;

  for (size_t cnum = 0; cnum < 16; cnum++) {
    for (int16_t mid = 0; mid < 3; mid++) {
      MetricBufferKey key;
      key.channel_number = channel_plane_numbers[cnum].first;
      key.metric_id = mid; // There are three processors each with one metric
      key.pipeline_id = 0;
      key.processor_id = mid;

      tpglibs::IndexAwareSignalPointer<__m256i> ptr;
      ptr.index = -1;
      ptr.valueptr = nullptr;

      table[key] = ptr;
    }
  }

  for (const __m256i& signal : signals) {
    std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps = pipeline2.process(signal);
    if (tps.empty()) continue;
  }

  pipeline2.get_pipeline_metrics(table, 0);

  bool pointers_are_not_null = true;
  bool index_are_assigned = true;
  bool correct_pedestal = true; // This is dependent on initialization value in Processor

  std::cout << "--- Metrics Table Contents ---\n";

  for (const auto& kv : table) {
      const auto& key = kv.first;
      const auto& ptr = kv.second;
      std::cout << "Key(channel=" << key.channel_number
                << ", metric=" << key.metric_id
                << ", pipeline=" << key.pipeline_id
                << ", processor=" << key.processor_id
                << ")  ";

      std::cout << "index=" << ptr.index << "  ";

      if (ptr.valueptr) {
        int16_t vals[16];
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(vals), *ptr.valueptr);
        std::cout << "values=[";
        for (int i = 0; i < 16; ++i) {
          std::cout << vals[i] << (i + 1 < 16 ? "," : "");
        }
        std::cout << "]";
      } else {
        std::cout << "valueptr=null";
      }
      std::cout << "\n";
  }
  std::cout << "------------------------------\n";

  for (size_t cnum = 0; cnum < 16; cnum++) {
    for (int16_t mid = 0; mid < 3; mid++) {
      MetricBufferKey key;
      key.channel_number = channel_plane_numbers[cnum].first;
      key.metric_id = 0;
      key.pipeline_id = 0;
      key.processor_id = 0;

      auto ptr = table.at(key);

      if (ptr.index == -1) {
        index_are_assigned = false;
        std::cout<<std::to_string(ptr.index)<<std::endl;
      } 

      if (ptr.valueptr == nullptr) {
        pointers_are_not_null = false;
        correct_pedestal = false;
      } else {
        int16_t vals[16];
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(vals), *ptr.valueptr);

        for (auto& val : vals) {
          if (val != 16384) { // Because we initialize at 0x4000
            correct_pedestal = false;
          } 
        }
      }
    }
  }

  BOOST_TEST(adc_peak_at_1600);
  BOOST_TEST(index_are_assigned);
  BOOST_TEST(pointers_are_not_null);
  BOOST_TEST(correct_pedestal);
}

} // namespace tpglibs
