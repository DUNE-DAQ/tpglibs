/**
 * @file avx_generator.cxx
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE boost_test_macro_overview

#include "tpgengine/TPGenerator.hpp"

#include <boost/test/included/unit_test.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>

namespace tpgengine {

BOOST_AUTO_TEST_CASE(test_macro_overview)
{
  // Build a frame to send through the TPGenerator.
  dunedaq::fddetdataformats::WIBEthFrame frame;

  const int num_time_samples_per_frame = dunedaq::fddetdataformats::WIBEthFrame::s_time_samples_per_frame;
  const int num_adc_words_per_ts = dunedaq::fddetdataformats::WIBEthFrame::s_num_adc_words_per_ts;

  for (int t_sample = 0; t_sample < num_time_samples_per_frame; t_sample++) {
    for (int chan = 0; chan < 64; chan++) {
      frame.set_adc(chan, t_sample, (t_sample*100 + chan) % 500);
    }
  }

  // Lazy with the plane assignments.
  int16_t plane_numbers[64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                               1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                               2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                               0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  // Just using the thresholding for simplicity.
  std::vector<nlohmann::json> configs = {
    {
      {"name", "AVXThresholdProcessor"},
      {"config",
        {
          {"plane0", 200},
          {"plane1", 300},
          {"plane2", 450}
        }
      }
    }
  };

  TPGenerator tpg;
  tpg.set_plane_numbers(plane_numbers);
  tpg.configure(configs);

  std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps = tpg(&frame);

  int16_t min_peak = INT16_MAX;
  size_t tp_count = tps.size();

  for (auto tp : tps) {
    if (tp.adc_peak < min_peak) min_peak = tp.adc_peak;
  }

  BOOST_TEST(tp_count == 576);
  BOOST_TEST(min_peak > 200);  // Truly it should depend on the plane, so this is naive.
}

} // namespace tpgengine
