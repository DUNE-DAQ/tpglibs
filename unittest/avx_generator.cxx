/**
 * @file avx_factory.cxx
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE boost_test_macro_overview
#define FMT_HEADER_ONLY

#include "tpgengine/AVXGenerator.hpp"

#include <boost/test/included/unit_test.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>

namespace tpgengine {

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

  AVXGenerator generator = AVXGenerator();

  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  // Horrendous brackets.
  std::vector<nlohmann::json> configs = {
    {
      {"name", "AVXRunSumProcessor"},
      {"config",
        {
          {"memory_factor_plane0", 10},
          {"memory_factor_plane1", 10},
          {"memory_factor_plane2", 10},
          {"scale_factor_plane0", 10},
          {"scale_factor_plane1", 10},
          {"scale_factor_plane2", 10},
        }
      }
    },
    {
      {"name", "AVXThresholdProcessor"},
      {"config",
        {
          {"plane0", 200},
          {"plane1", 1000},
          {"plane2", 1500}
        }
      }
    }
  };

  generator.set_plane_numbers(plane_numbers);
  generator.configure(configs);

  // ADC peak should max at 1600 for all channels.
  bool adc_peak_at_1600 = true;

  for (const __m256i& signal : signals) {
    std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps = generator.process(signal);
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

} // namespace tpgengine
