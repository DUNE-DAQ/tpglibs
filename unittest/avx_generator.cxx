/**
 * @file avx_generator.cxx
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE boost_test_macro_overview

#include "tpgengine/TPGenerator.hpp"

#include "fddetdataformats/WIBEthFrame.hpp"
#include "fddetdataformats/TDEEthFrame.hpp"

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

  // Lazy with the channel-plane assignments.
  std::vector<std::pair<int16_t, int16_t>> channel_plane_numbers = {{ 0, 0}, { 1, 0}, { 2, 0}, { 3, 0}, { 4, 0}, { 5, 0}, { 6, 0}, { 7, 0}, { 8, 0}, { 9, 0}, {10, 0}, {11, 0}, {12, 0}, {13, 0}, {14, 0}, {15, 0},
                                                                    {16, 1}, {17, 1}, {18, 1}, {19, 1}, {20, 1}, {21, 1}, {22, 1}, {23, 1}, {24, 1}, {25, 1}, {26, 1}, {27, 1}, {28, 1}, {29, 1}, {30, 1}, {31, 1},
                                                                    {32, 2}, {33, 2}, {34, 2}, {35, 2}, {36, 2}, {37, 2}, {38, 2}, {39, 2}, {40, 2}, {41, 2}, {42, 2}, {43, 2}, {44, 2}, {45, 2}, {46, 2}, {47, 2},
                                                                    {48, 0}, {49, 0}, {50, 0}, {51, 0}, {52, 0}, {53, 1}, {54, 1}, {55, 1}, {56, 1}, {57, 1}, {58, 2}, {59, 2}, {60, 2}, {61, 2}, {62, 2}, {63, 2}};

  // Just using the thresholding for simplicity.
  std::vector<nlohmann::json> configs = {
    {
      {"name", "AVXThresholdProcessor"},
      {"config",
        {
          {"plane0", 200},
          {"plane1", 300},
          {"plane2", 445}
        }
      }
    }
  };

  TPGenerator tpg;
  tpg.configure(configs, channel_plane_numbers);

  std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps = tpg(&frame);

  int16_t min_peak = INT16_MAX;
  size_t tp_count = tps.size();

  for (auto tp : tps) {
    if (tp.adc_peak < min_peak) min_peak = tp.adc_peak;
  }

  BOOST_TEST(tp_count == 600);
  BOOST_TEST(min_peak > 200);  // Truly it should depend on the plane, so this is naive.
}

} // namespace tpgengine
