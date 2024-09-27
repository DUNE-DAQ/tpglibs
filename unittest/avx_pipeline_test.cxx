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

#include <boost/test/unit_test.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>

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

  std::vector<std::pair<int16_t, int16_t>> channel_plane_numbers = {{  0, 0},
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

  pipeline.configure(configs, channel_plane_numbers);

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

  BOOST_TEST(adc_peak_at_1600);
}

} // namespace tpglibs
