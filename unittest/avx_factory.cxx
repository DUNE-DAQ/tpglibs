/**
 * @file avx_factory.cxx
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE boost_test_macro_overview
#define FMT_HEADER_ONLY

#include "tpglibs/AVXFactory.hpp"

#include <boost/test/included/unit_test.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>

namespace tpglibs {

BOOST_AUTO_TEST_CASE(test_macro_overview)
{
  // This is the focus test. The rest is just to make something interesting to pass.
  auto avx_factory = AVXFactory::get_instance();
  std::shared_ptr<AVXProcessor> threshold = avx_factory->create_processor("AVXThresholdProcessor");
  std::shared_ptr<AVXProcessor> abs_rs = avx_factory->create_processor("AVXAbsRunSumProcessor");
  std::shared_ptr<AVXProcessor> rs = avx_factory->create_processor("AVXRunSumProcessor");

  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  nlohmann::json thr_config = {
    {"plane0", 300},
    {"plane1", 1000},
    {"plane2", 1600}
  };
  threshold->configure(thr_config, plane_numbers);

  nlohmann::json rs_config = {
    {"memory_factor_plane0", 8},
    {"memory_factor_plane1", 8},
    {"memory_factor_plane2", 8},
    {"scale_factor_plane0", 5},
    {"scale_factor_plane1", 5},
    {"scale_factor_plane2", 5}
  };
  abs_rs->configure(rs_config, plane_numbers);
  rs->configure(rs_config, plane_numbers);

  abs_rs->set_next_processor(threshold);
  rs->set_next_processor(threshold);

  // Arbitrary input choices. Set so RS resets to 0.
  __m256i input0 = _mm256_set_epi16(-1600, 1500, -1400, 1300, -1200, 1100, -1000,  900,
                                     -800,  700,  -600,  500,  -400,  300,  -200,  100);
  __m256i input1 = _mm256_set_epi16( 2880,-2700,  2520,-2340,  2160,-1980,  1800,-1620,
                                     1440,-1260,  1080, -900,   720, -540,   360, -180);
  __m256i input2 = _mm256_set_epi16(-1280, 1200, -1120, 1040,  -960,  880,  -800,  720,
                                     -640,  560,  -480,  400,  -320,  240,  -160,   80);

  __m256i avx_abs_output = abs_rs->process(input0);
  avx_abs_output = abs_rs->process(input1);
  avx_abs_output = abs_rs->process(input2);

  __m256i avx_rs_output = rs->process(input0);
  avx_rs_output = rs->process(input1);
  avx_rs_output = rs->process(input2);

  int16_t abs_output[16], rs_output[16];
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(&abs_output), avx_abs_output);
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(&rs_output),  avx_rs_output);

  bool same_abs = true;
  bool same_rs  = true;

  int16_t expected_abs[16] = {   0,    0,  432,  576,  720,    0, 1008, 1152,
                              1296, 1440,    0, 1728, 1872, 2016, 2160, 2304};
  int16_t expected_rs[16]  = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  for (int i = 0; i < 16; i++) {
    if (expected_abs[i] != abs_output[i]) same_abs = false;
    if (expected_rs[i] != rs_output[i]) same_rs = false;
  }

//  fmt::print("AbsRS: [{:5}]\n", fmt::join(abs_output, ","));
//  fmt::print("RS:    [{:5}]\n", fmt::join(rs_output, ","));

  BOOST_TEST(same_abs);
  BOOST_TEST(same_rs);
}

} // namespace tpglibs
