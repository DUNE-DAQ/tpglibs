/**
 * @file processor_state_registrate_collect_test.cxx
 *
 * @brief Comprehensive unit tests for internal state registration and collection
 *        across all processors that support internal state monitoring
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifdef TPGLIBS_ENABLE_STATE_MONITORING

#define BOOST_TEST_MODULE ProcessorStateRegistrationCollectionTest

#include "tpglibs/AVXRunSumProcessor.hpp"
#include "tpglibs/AVXAbsRunSumProcessor.hpp"
#include "tpglibs/AVXFrugalPedestalSubtractProcessor.hpp"
#include "tpglibs/NaiveRunSumProcessor.hpp"
#include "tpglibs/NaiveAbsRunSumProcessor.hpp"
#include "tpglibs/NaiveFrugalPedestalSubtractProcessor.hpp"

#include <boost/test/unit_test.hpp>
#include <immintrin.h>
#include <array>
#include <vector>
#include <memory>

namespace tpglibs {

// =============================================================================
// Helper Functions
// =============================================================================

// Helper function to create test signal for AVX processors
__m256i create_avx_test_signal(const int16_t values[16]) {
  return _mm256_set_epi16(
    values[15], values[14], values[13], values[12],
    values[11], values[10], values[9], values[8],
    values[7], values[6], values[5], values[4],
    values[3], values[2], values[1], values[0]
  );
}

// Helper function to create test signal for Naive processors
std::array<int16_t, 16> create_naive_test_signal(const int16_t values[16]) {
  std::array<int16_t, 16> signal;
  for (int i = 0; i < 16; ++i) {
    signal[i] = values[i];
  }
  return signal;
}

// Helper function to extract values from __m256i
void extract_avx_values(__m256i vec, int16_t output[16]) {
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(output), vec);
}

// Helper function to create standard test configuration
nlohmann::json create_test_config(const std::string& requested_states = "") {
  nlohmann::json config = {
    {"metric_collect_toggle_state", true},
    {"metric_collect_time_sample_period", 1}  // Collect every sample
  };
  
  if (!requested_states.empty()) {
    config["requested_internal_states"] = requested_states;
  }
  
  return config;
}

// =============================================================================
// SECTION 1: AVXRunSumProcessor Tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(AVXRunSumProcessorTests)

BOOST_AUTO_TEST_CASE(test_avx_runsum_configuration_and_registration) {
  auto processor = std::make_shared<AVXRunSumProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "r,s,rs";
  config["memory_factor_plane0"] = 100;
  config["memory_factor_plane1"] = 200;
  config["memory_factor_plane2"] = 300;
  config["scale_factor_plane0"] = 10;
  config["scale_factor_plane1"] = 20;
  config["scale_factor_plane2"] = 30;
  config["memory_divisor_plane0"] = 10;
  config["memory_divisor_plane1"] = 10;
  config["memory_divisor_plane2"] = 10;
  config["scale_divisor_plane0"] = 10;
  config["scale_divisor_plane1"] = 10;
  config["scale_divisor_plane2"] = 10;

  processor->configure(config, plane_numbers);

  // Test that internal states are registered
  auto requested_states = processor->get_requested_internal_state_names();
  BOOST_TEST(requested_states.size() == 3);
  BOOST_TEST(requested_states[0] == "r");
  BOOST_TEST(requested_states[1] == "s");
  BOOST_TEST(requested_states[2] == "rs");
}

BOOST_AUTO_TEST_CASE(test_avx_runsum_initial_state_collection) {
  auto processor = std::make_shared<AVXRunSumProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "r,s,rs";
  config["memory_factor_plane0"] = 100;
  config["memory_factor_plane1"] = 200;
  config["memory_factor_plane2"] = 300;
  config["scale_factor_plane0"] = 10;
  config["scale_factor_plane1"] = 20;
  config["scale_factor_plane2"] = 30;
  config["memory_divisor_plane0"] = 10;
  config["memory_divisor_plane1"] = 10;
  config["memory_divisor_plane2"] = 10;
  config["scale_divisor_plane0"] = 10;
  config["scale_divisor_plane1"] = 10;
  config["scale_divisor_plane2"] = 10;

  processor->configure(config, plane_numbers);

  // Collect initial state
  auto buffer_manager = processor->_get_internal_state_buffer_manager();
  buffer_manager->write_to_active_buffer();
  auto state_value = buffer_manager->switch_buffer_and_read_casted();

  BOOST_TEST(state_value.m_size == 3);
  BOOST_TEST(state_value.m_data != nullptr);

  // Check initial values
  // r (memory_factor) should be configured values
  int16_t expected_r[16] = {100, 100, 100, 100, 100, 200, 200, 200, 200, 200, 300, 300, 300, 300, 300, 300};
  for (int i = 0; i < 16; ++i) {
    BOOST_TEST(state_value.m_data[0][i] == expected_r[i]);
  }

  // s (scale_factor) should be configured values
  int16_t expected_s[16] = {10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 30, 30, 30, 30, 30, 30};
  for (int i = 0; i < 16; ++i) {
    BOOST_TEST(state_value.m_data[1][i] == expected_s[i]);
  }

  // rs (running_sum) should be initialized to 0
  for (int i = 0; i < 16; ++i) {
    BOOST_TEST(state_value.m_data[2][i] == 0);
  }
}

BOOST_AUTO_TEST_CASE(test_avx_runsum_processing_and_state_update) {
  auto processor = std::make_shared<AVXRunSumProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "rs";  // Only collect running sum
  config["memory_factor_plane0"] = 100;
  config["memory_factor_plane1"] = 200;
  config["memory_factor_plane2"] = 300;
  config["scale_factor_plane0"] = 10;
  config["scale_factor_plane1"] = 20;
  config["scale_factor_plane2"] = 30;
  config["memory_divisor_plane0"] = 10;
  config["memory_divisor_plane1"] = 10;
  config["memory_divisor_plane2"] = 10;
  config["scale_divisor_plane0"] = 10;
  config["scale_divisor_plane1"] = 10;
  config["scale_divisor_plane2"] = 10;

  processor->configure(config, plane_numbers);

  // Create test signal
  int16_t signal_values[16];
  for (int i = 0; i < 16; ++i) {
    signal_values[i] = 1000;  // Constant signal
  }
  __m256i signal = create_avx_test_signal(signal_values);

  // Process the signal twice - first call writes old state to buffer, second call updates state
  processor->process(signal);
  processor->process(signal);

  // Collect state after processing
  auto buffer_manager = processor->_get_internal_state_buffer_manager();
  auto state_value = buffer_manager->switch_buffer_and_read_casted();

  BOOST_TEST(state_value.m_size == 1);
  BOOST_TEST(state_value.m_data != nullptr);

  // Running sum should have been updated (non-zero)
  bool running_sum_updated = false;
  for (int i = 0; i < 16; ++i) {
    if (state_value.m_data[0][i] != 0) {
      running_sum_updated = true;
      break;
    }
  }
  BOOST_TEST(running_sum_updated);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// SECTION 2: AVXAbsRunSumProcessor Tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(AVXAbsRunSumProcessorTests)

BOOST_AUTO_TEST_CASE(test_avx_abs_runsum_inherits_registration) {
  auto processor = std::make_shared<AVXAbsRunSumProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "r,s,rs";
  config["memory_factor_plane0"] = 100;
  config["memory_factor_plane1"] = 200;
  config["memory_factor_plane2"] = 300;
  config["scale_factor_plane0"] = 10;
  config["scale_factor_plane1"] = 20;
  config["scale_factor_plane2"] = 30;
  config["memory_divisor_plane0"] = 10;
  config["memory_divisor_plane1"] = 10;
  config["memory_divisor_plane2"] = 10;
  config["scale_divisor_plane0"] = 10;
  config["scale_divisor_plane1"] = 10;
  config["scale_divisor_plane2"] = 10;

  processor->configure(config, plane_numbers);

  // Should inherit the same internal state registration as parent
  auto requested_states = processor->get_requested_internal_state_names();
  BOOST_TEST(requested_states.size() == 3);
  BOOST_TEST(requested_states[0] == "r");
  BOOST_TEST(requested_states[1] == "s");
  BOOST_TEST(requested_states[2] == "rs");
}

BOOST_AUTO_TEST_CASE(test_avx_abs_runsum_processing_with_absolute_values) {
  auto processor = std::make_shared<AVXAbsRunSumProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "rs";
  config["memory_factor_plane0"] = 100;
  config["memory_factor_plane1"] = 200;
  config["memory_factor_plane2"] = 300;
  config["scale_factor_plane0"] = 10;
  config["scale_factor_plane1"] = 20;
  config["scale_factor_plane2"] = 30;
  config["memory_divisor_plane0"] = 10;
  config["memory_divisor_plane1"] = 10;
  config["memory_divisor_plane2"] = 10;
  config["scale_divisor_plane0"] = 10;
  config["scale_divisor_plane1"] = 10;
  config["scale_divisor_plane2"] = 10;

  processor->configure(config, plane_numbers);

  // Create test signal with negative values
  int16_t signal_values[16];
  for (int i = 0; i < 16; ++i) {
    signal_values[i] = -1000;  // Negative signal
  }
  __m256i signal = create_avx_test_signal(signal_values);

  // Process the signal twice - first call writes old state to buffer, second call updates state
  processor->process(signal);
  processor->process(signal);

  // Collect state after processing
  auto buffer_manager = processor->_get_internal_state_buffer_manager();
  auto state_value = buffer_manager->switch_buffer_and_read_casted();

  BOOST_TEST(state_value.m_size == 1);
  BOOST_TEST(state_value.m_data != nullptr);

  // Running sum should have been updated (non-zero)
  bool running_sum_updated = false;
  for (int i = 0; i < 16; ++i) {
    if (state_value.m_data[0][i] != 0) {
      running_sum_updated = true;
      break;
    }
  }
  BOOST_TEST(running_sum_updated);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// SECTION 3: NaiveRunSumProcessor Tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(NaiveRunSumProcessorTests)

BOOST_AUTO_TEST_CASE(test_naive_runsum_configuration_and_registration) {
  auto processor = std::make_shared<NaiveRunSumProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "r,s,rs";
  config["memory_factor_plane0"] = 100;
  config["memory_factor_plane1"] = 200;
  config["memory_factor_plane2"] = 300;
  config["scale_factor_plane0"] = 10;
  config["scale_factor_plane1"] = 20;
  config["scale_factor_plane2"] = 30;
  config["memory_divisor_plane0"] = 10;
  config["memory_divisor_plane1"] = 10;
  config["memory_divisor_plane2"] = 10;
  config["scale_divisor_plane0"] = 10;
  config["scale_divisor_plane1"] = 10;
  config["scale_divisor_plane2"] = 10;

  processor->configure(config, plane_numbers);

  // Test that internal states are registered
  auto requested_states = processor->get_requested_internal_state_names();
  BOOST_TEST(requested_states.size() == 3);
  BOOST_TEST(requested_states[0] == "r");
  BOOST_TEST(requested_states[1] == "s");
  BOOST_TEST(requested_states[2] == "rs");
}

BOOST_AUTO_TEST_CASE(test_naive_runsum_initial_state_collection) {
  auto processor = std::make_shared<NaiveRunSumProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "r,s,rs";
  config["memory_factor_plane0"] = 100;
  config["memory_factor_plane1"] = 200;
  config["memory_factor_plane2"] = 300;
  config["scale_factor_plane0"] = 10;
  config["scale_factor_plane1"] = 20;
  config["scale_factor_plane2"] = 30;
  config["memory_divisor_plane0"] = 10;
  config["memory_divisor_plane1"] = 10;
  config["memory_divisor_plane2"] = 10;
  config["scale_divisor_plane0"] = 10;
  config["scale_divisor_plane1"] = 10;
  config["scale_divisor_plane2"] = 10;

  processor->configure(config, plane_numbers);

  // Collect initial state
  auto buffer_manager = processor->_get_internal_state_buffer_manager();
  buffer_manager->write_to_active_buffer();
  auto state_value = buffer_manager->switch_buffer_and_read_casted();

  BOOST_TEST(state_value.m_size == 3);
  BOOST_TEST(state_value.m_data != nullptr);

  // Check initial values
  // r (memory_factor) should be configured values
  int16_t expected_r[16] = {100, 100, 100, 100, 100, 200, 200, 200, 200, 200, 300, 300, 300, 300, 300, 300};
  for (int i = 0; i < 16; ++i) {
    BOOST_TEST(state_value.m_data[0][i] == expected_r[i]);
  }

  // s (scale_factor) should be configured values
  int16_t expected_s[16] = {10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 30, 30, 30, 30, 30, 30};
  for (int i = 0; i < 16; ++i) {
    BOOST_TEST(state_value.m_data[1][i] == expected_s[i]);
  }

  // rs (running_sum) should be initialized to 0
  for (int i = 0; i < 16; ++i) {
    BOOST_TEST(state_value.m_data[2][i] == 0);
  }
}

BOOST_AUTO_TEST_CASE(test_naive_runsum_processing_and_state_update) {
  auto processor = std::make_shared<NaiveRunSumProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "rs";  // Only collect running sum
  config["memory_factor_plane0"] = 100;
  config["memory_factor_plane1"] = 200;
  config["memory_factor_plane2"] = 300;
  config["scale_factor_plane0"] = 10;
  config["scale_factor_plane1"] = 20;
  config["scale_factor_plane2"] = 30;
  config["memory_divisor_plane0"] = 10;
  config["memory_divisor_plane1"] = 10;
  config["memory_divisor_plane2"] = 10;
  config["scale_divisor_plane0"] = 10;
  config["scale_divisor_plane1"] = 10;
  config["scale_divisor_plane2"] = 10;

  processor->configure(config, plane_numbers);

  // Create test signal
  int16_t signal_values[16];
  for (int i = 0; i < 16; ++i) {
    signal_values[i] = 1000;  // Constant signal
  }
  auto signal = create_naive_test_signal(signal_values);

  // Process the signal twice - first call writes old state to buffer, second call updates state
  processor->process(signal);
  processor->process(signal);

  // Collect state after processing
  auto buffer_manager = processor->_get_internal_state_buffer_manager();
  auto state_value = buffer_manager->switch_buffer_and_read_casted();

  BOOST_TEST(state_value.m_size == 1);
  BOOST_TEST(state_value.m_data != nullptr);

  // Running sum should have been updated (non-zero)
  bool running_sum_updated = false;
  for (int i = 0; i < 16; ++i) {
    if (state_value.m_data[0][i] != 0) {
      running_sum_updated = true;
      break;
    }
  }
  BOOST_TEST(running_sum_updated);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// SECTION 4: AVXFrugalPedestalSubtractProcessor Tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(AVXFrugalPedestalSubtractProcessorTests)

BOOST_AUTO_TEST_CASE(test_avx_frugal_configuration_and_registration) {
  auto processor = std::make_shared<AVXFrugalPedestalSubtractProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "pedestal,accum";
  config["accum_limit"] = 10;

  processor->configure(config, plane_numbers);

  // Test that internal states are registered
  auto requested_states = processor->get_requested_internal_state_names();
  BOOST_TEST(requested_states.size() == 2);
  BOOST_TEST(requested_states[0] == "pedestal");
  BOOST_TEST(requested_states[1] == "accum");
}

BOOST_AUTO_TEST_CASE(test_avx_frugal_initial_state_collection) {
  auto processor = std::make_shared<AVXFrugalPedestalSubtractProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "pedestal,accum";
  config["accum_limit"] = 10;

  processor->configure(config, plane_numbers);

  // Collect initial state
  auto buffer_manager = processor->_get_internal_state_buffer_manager();
  buffer_manager->write_to_active_buffer();
  auto state_value = buffer_manager->switch_buffer_and_read_casted();

  BOOST_TEST(state_value.m_size == 2);
  BOOST_TEST(state_value.m_data != nullptr);

  // Check initial values
  // pedestal should be initialized to 0x4000 (16384) - 14-bit max
  for (int i = 0; i < 16; ++i) {
    BOOST_TEST(state_value.m_data[0][i] == 0x4000);
  }

  // accum should be initialized to 0
  for (int i = 0; i < 16; ++i) {
    BOOST_TEST(state_value.m_data[1][i] == 0);
  }
}

BOOST_AUTO_TEST_CASE(test_avx_frugal_processing_and_state_update) {
  auto processor = std::make_shared<AVXFrugalPedestalSubtractProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "pedestal,accum";
  config["accum_limit"] = 3;  // Small limit for faster adaptation

  processor->configure(config, plane_numbers);

  // Create test signal consistently above initial pedestal
  int16_t signal_values[16];
  for (int i = 0; i < 16; ++i) {
    signal_values[i] = 0x5000;  // Above initial pedestal (0x4000)
  }
  __m256i signal = create_avx_test_signal(signal_values);

  // Process multiple times to trigger adaptation
  for (int i = 0; i < 20; ++i) {
    processor->process(signal);
  }

  // Collect state after processing
  auto buffer_manager = processor->_get_internal_state_buffer_manager();
  auto state_value = buffer_manager->switch_buffer_and_read_casted();

  BOOST_TEST(state_value.m_size == 2);
  BOOST_TEST(state_value.m_data != nullptr);

  // With signal consistently above pedestal and small accum_limit,
  // pedestal should have increased
  bool pedestal_increased = false;
  for (int i = 0; i < 16; ++i) {
    if (state_value.m_data[0][i] > 0x4000) {
      pedestal_increased = true;
      break;
    }
  }
  BOOST_TEST(pedestal_increased);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// SECTION 5: NaiveAbsRunSumProcessor Tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(NaiveAbsRunSumProcessorTests)

BOOST_AUTO_TEST_CASE(test_naive_abs_runsum_inherits_registration) {
  auto processor = std::make_shared<NaiveAbsRunSumProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "r,s,rs";
  config["memory_factor_plane0"] = 100;
  config["memory_factor_plane1"] = 200;
  config["memory_factor_plane2"] = 300;
  config["scale_factor_plane0"] = 10;
  config["scale_factor_plane1"] = 20;
  config["scale_factor_plane2"] = 30;
  config["memory_divisor_plane0"] = 10;
  config["memory_divisor_plane1"] = 10;
  config["memory_divisor_plane2"] = 10;
  config["scale_divisor_plane0"] = 10;
  config["scale_divisor_plane1"] = 10;
  config["scale_divisor_plane2"] = 10;

  processor->configure(config, plane_numbers);

  // Should inherit the same internal state registration as parent
  auto requested_states = processor->get_requested_internal_state_names();
  BOOST_TEST(requested_states.size() == 3);
  BOOST_TEST(requested_states[0] == "r");
  BOOST_TEST(requested_states[1] == "s");
  BOOST_TEST(requested_states[2] == "rs");
}

BOOST_AUTO_TEST_CASE(test_naive_abs_runsum_processing_with_absolute_values) {
  auto processor = std::make_shared<NaiveAbsRunSumProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "rs";
  config["memory_factor_plane0"] = 100;
  config["memory_factor_plane1"] = 200;
  config["memory_factor_plane2"] = 300;
  config["scale_factor_plane0"] = 10;
  config["scale_factor_plane1"] = 20;
  config["scale_factor_plane2"] = 30;
  config["memory_divisor_plane0"] = 10;
  config["memory_divisor_plane1"] = 10;
  config["memory_divisor_plane2"] = 10;
  config["scale_divisor_plane0"] = 10;
  config["scale_divisor_plane1"] = 10;
  config["scale_divisor_plane2"] = 10;

  processor->configure(config, plane_numbers);

  // Create test signal with negative values
  int16_t signal_values[16];
  for (int i = 0; i < 16; ++i) {
    signal_values[i] = -1000;  // Negative signal
  }
  auto signal = create_naive_test_signal(signal_values);

  // Process the signal twice - first call writes old state to buffer, second call updates state
  processor->process(signal);
  processor->process(signal);

  // Collect state after processing
  auto buffer_manager = processor->_get_internal_state_buffer_manager();
  auto state_value = buffer_manager->switch_buffer_and_read_casted();

  BOOST_TEST(state_value.m_size == 1);
  BOOST_TEST(state_value.m_data != nullptr);

  // Running sum should have been updated (non-zero)
  bool running_sum_updated = false;
  for (int i = 0; i < 16; ++i) {
    if (state_value.m_data[0][i] != 0) {
      running_sum_updated = true;
      break;
    }
  }
  BOOST_TEST(running_sum_updated);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// SECTION 6: NaiveFrugalPedestalSubtractProcessor Tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(NaiveFrugalPedestalSubtractProcessorTests)

BOOST_AUTO_TEST_CASE(test_naive_frugal_configuration_and_registration) {
  auto processor = std::make_shared<NaiveFrugalPedestalSubtractProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "pedestal,accum";
  config["accum_limit"] = 10;

  processor->configure(config, plane_numbers);

  // Test that internal states are registered
  auto requested_states = processor->get_requested_internal_state_names();
  BOOST_TEST(requested_states.size() == 2);
  BOOST_TEST(requested_states[0] == "pedestal");
  BOOST_TEST(requested_states[1] == "accum");
}

BOOST_AUTO_TEST_CASE(test_naive_frugal_initial_state_collection) {
  auto processor = std::make_shared<NaiveFrugalPedestalSubtractProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "pedestal,accum";
  config["accum_limit"] = 10;

  processor->configure(config, plane_numbers);

  // Collect initial state
  auto buffer_manager = processor->_get_internal_state_buffer_manager();
  buffer_manager->write_to_active_buffer();
  auto state_value = buffer_manager->switch_buffer_and_read_casted();

  BOOST_TEST(state_value.m_size == 2);
  BOOST_TEST(state_value.m_data != nullptr);

  // Check initial values
  // pedestal should be initialized to 0
  for (int i = 0; i < 16; ++i) {
    BOOST_TEST(state_value.m_data[0][i] == 0);
  }

  // accum should be initialized to 0
  for (int i = 0; i < 16; ++i) {
    BOOST_TEST(state_value.m_data[1][i] == 0);
  }
}

BOOST_AUTO_TEST_CASE(test_naive_frugal_processing_and_state_update) {
  auto processor = std::make_shared<NaiveFrugalPedestalSubtractProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "pedestal,accum";
  config["accum_limit"] = 3;  // Small limit for faster adaptation

  processor->configure(config, plane_numbers);

  // Create test signal consistently above initial pedestal
  int16_t signal_values[16];
  for (int i = 0; i < 16; ++i) {
    signal_values[i] = 1000;  // Above initial pedestal (0)
  }
  auto signal = create_naive_test_signal(signal_values);

  // Process multiple times to trigger adaptation
  for (int i = 0; i < 10; ++i) {
    processor->process(signal);
  }

  // Collect state after processing
  auto buffer_manager = processor->_get_internal_state_buffer_manager();
  auto state_value = buffer_manager->switch_buffer_and_read_casted();

  BOOST_TEST(state_value.m_size == 2);
  BOOST_TEST(state_value.m_data != nullptr);

  // With signal consistently above pedestal and small accum_limit,
  // pedestal should have increased
  bool pedestal_increased = false;
  for (int i = 0; i < 16; ++i) {
    if (state_value.m_data[0][i] > 0) {
      pedestal_increased = true;
      break;
    }
  }
  BOOST_TEST(pedestal_increased);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// SECTION 7: Cross-Processor Integration Tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(CrossProcessorIntegrationTests)

BOOST_AUTO_TEST_CASE(test_all_processors_independence) {
  // Create all processor types
  auto avx_runsum = std::make_shared<AVXRunSumProcessor>();
  auto avx_abs_runsum = std::make_shared<AVXAbsRunSumProcessor>();
  auto avx_frugal = std::make_shared<AVXFrugalPedestalSubtractProcessor>();
  auto naive_runsum = std::make_shared<NaiveRunSumProcessor>();
  auto naive_abs_runsum = std::make_shared<NaiveAbsRunSumProcessor>();
  auto naive_frugal = std::make_shared<NaiveFrugalPedestalSubtractProcessor>();

  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  // Configure all processors
  auto config = create_test_config();
  config["memory_factor_plane0"] = 100;
  config["memory_factor_plane1"] = 200;
  config["memory_factor_plane2"] = 300;
  config["scale_factor_plane0"] = 10;
  config["scale_factor_plane1"] = 20;
  config["scale_factor_plane2"] = 30;
  config["memory_divisor_plane0"] = 10;
  config["memory_divisor_plane1"] = 10;
  config["memory_divisor_plane2"] = 10;
  config["scale_divisor_plane0"] = 10;
  config["scale_divisor_plane1"] = 10;
  config["scale_divisor_plane2"] = 10;
  config["accum_limit"] = 10;

  // Configure each processor with its specific states
  auto avx_runsum_config = config;
  avx_runsum_config["requested_internal_states"] = "r,s,rs";
  avx_runsum->configure(avx_runsum_config, plane_numbers);
  
  auto avx_abs_runsum_config = config;
  avx_abs_runsum_config["requested_internal_states"] = "r,s,rs";
  avx_abs_runsum->configure(avx_abs_runsum_config, plane_numbers);
  
  auto avx_frugal_config = config;
  avx_frugal_config["requested_internal_states"] = "pedestal,accum";
  avx_frugal->configure(avx_frugal_config, plane_numbers);
  
  auto naive_runsum_config = config;
  naive_runsum_config["requested_internal_states"] = "r,s,rs";
  naive_runsum->configure(naive_runsum_config, plane_numbers);
  
  auto naive_abs_runsum_config = config;
  naive_abs_runsum_config["requested_internal_states"] = "r,s,rs";
  naive_abs_runsum->configure(naive_abs_runsum_config, plane_numbers);
  
  auto naive_frugal_config = config;
  naive_frugal_config["requested_internal_states"] = "pedestal,accum";
  naive_frugal->configure(naive_frugal_config, plane_numbers);

  // Test that all processors have their expected internal states
  auto avx_runsum_states = avx_runsum->get_requested_internal_state_names();
  auto avx_abs_runsum_states = avx_abs_runsum->get_requested_internal_state_names();
  auto avx_frugal_states = avx_frugal->get_requested_internal_state_names();
  auto naive_runsum_states = naive_runsum->get_requested_internal_state_names();
  auto naive_abs_runsum_states = naive_abs_runsum->get_requested_internal_state_names();
  auto naive_frugal_states = naive_frugal->get_requested_internal_state_names();

  BOOST_TEST(avx_runsum_states.size() == 3);
  BOOST_TEST(avx_abs_runsum_states.size() == 3);
  BOOST_TEST(avx_frugal_states.size() == 2);
  BOOST_TEST(naive_runsum_states.size() == 3);
  BOOST_TEST(naive_abs_runsum_states.size() == 3);
  BOOST_TEST(naive_frugal_states.size() == 2);

  // Test that they are independent
  auto bm1 = avx_runsum->_get_internal_state_buffer_manager();
  auto bm2 = avx_frugal->_get_internal_state_buffer_manager();
  auto bm3 = naive_runsum->_get_internal_state_buffer_manager();
  auto bm4 = naive_frugal->_get_internal_state_buffer_manager();

  bm1->write_to_active_buffer();
  bm2->write_to_active_buffer();
  bm3->write_to_active_buffer();
  bm4->write_to_active_buffer();

  auto state1 = bm1->switch_buffer_and_read_casted();
  auto state2 = bm2->switch_buffer_and_read_casted();
  auto state3 = bm3->switch_buffer_and_read_casted();
  auto state4 = bm4->switch_buffer_and_read_casted();

  BOOST_TEST(state1.m_size == 3);
  BOOST_TEST(state2.m_size == 2);
  BOOST_TEST(state3.m_size == 3);
  BOOST_TEST(state4.m_size == 2);

  // They should have different buffer managers (compare within same type)
  BOOST_TEST(bm1 != bm2);
  BOOST_TEST(bm3 != bm4);
  // Note: Can't compare AVX and Naive buffer managers directly due to different types
}

BOOST_AUTO_TEST_CASE(test_processor_chain_with_internal_states) {
  // Create a chain of processors (same type to avoid template issues)
  auto avx_runsum1 = std::make_shared<AVXRunSumProcessor>();
  auto avx_runsum2 = std::make_shared<AVXRunSumProcessor>();

  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "r,s,rs";
  config["memory_factor_plane0"] = 100;
  config["memory_factor_plane1"] = 200;
  config["memory_factor_plane2"] = 300;
  config["scale_factor_plane0"] = 10;
  config["scale_factor_plane1"] = 20;
  config["scale_factor_plane2"] = 30;
  config["memory_divisor_plane0"] = 10;
  config["memory_divisor_plane1"] = 10;
  config["memory_divisor_plane2"] = 10;
  config["scale_divisor_plane0"] = 10;
  config["scale_divisor_plane1"] = 10;
  config["scale_divisor_plane2"] = 10;

  avx_runsum1->configure(config, plane_numbers);
  avx_runsum2->configure(config, plane_numbers);

  // Chain them together
  avx_runsum1->set_next_processor(avx_runsum2);

  // Test that both processors maintain their internal state registration
  auto avx_states1 = avx_runsum1->get_requested_internal_state_names();
  auto avx_states2 = avx_runsum2->get_requested_internal_state_names();

  BOOST_TEST(avx_states1.size() == 3);
  BOOST_TEST(avx_states2.size() == 3);

  // Process through the chain
  int16_t signal_values[16];
  for (int i = 0; i < 16; ++i) {
    signal_values[i] = 1000;
  }
  __m256i signal = create_avx_test_signal(signal_values);

  avx_runsum1->process(signal);

  // Both processors should have updated their internal states
  auto bm1 = avx_runsum1->_get_internal_state_buffer_manager();
  auto bm2 = avx_runsum2->_get_internal_state_buffer_manager();

  auto state1 = bm1->switch_buffer_and_read_casted();
  auto state2 = bm2->switch_buffer_and_read_casted();

  BOOST_TEST(state1.m_size == 3);
  BOOST_TEST(state2.m_size == 3);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// SECTION 8: Basic Edge Cases (Minimal Coverage)
// =============================================================================

BOOST_AUTO_TEST_SUITE(BasicEdgeCases)

BOOST_AUTO_TEST_CASE(test_collection_disabled) {
  auto processor = std::make_shared<AVXRunSumProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  nlohmann::json config = {
    {"memory_factor_plane0", 100},
    {"memory_factor_plane1", 200},
    {"memory_factor_plane2", 300},
    {"scale_factor_plane0", 10},
    {"scale_factor_plane1", 20},
    {"scale_factor_plane2", 30},
    {"memory_divisor_plane0", 10},
    {"memory_divisor_plane1", 10},
    {"memory_divisor_plane2", 10},
    {"scale_divisor_plane0", 10},
    {"scale_divisor_plane1", 10},
    {"scale_divisor_plane2", 10},
    {"metric_collect_toggle_state", false}
  };

  processor->configure(config, plane_numbers);

  auto requested_states = processor->get_requested_internal_state_names();
  BOOST_TEST(requested_states.size() == 0);

  auto buffer_manager = processor->_get_internal_state_buffer_manager();
  buffer_manager->write_to_active_buffer();
  auto state_value = buffer_manager->switch_buffer_and_read_casted();

  BOOST_TEST(state_value.m_size == 0);
}

BOOST_AUTO_TEST_CASE(test_empty_internal_state_request) {
  auto processor = std::make_shared<NaiveFrugalPedestalSubtractProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["requested_internal_states"] = "";
  config["accum_limit"] = 10;

  processor->configure(config, plane_numbers);

  auto requested_states = processor->get_requested_internal_state_names();
  BOOST_TEST(requested_states.size() == 0);

  auto buffer_manager = processor->_get_internal_state_buffer_manager();
  buffer_manager->write_to_active_buffer();
  auto state_value = buffer_manager->switch_buffer_and_read_casted();

  BOOST_TEST(state_value.m_size == 0);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// SECTION 9: Core Registration and Collection Verification
// =============================================================================

BOOST_AUTO_TEST_SUITE(CoreRegistrationAndCollectionVerification)

BOOST_AUTO_TEST_CASE(test_all_processors_core_functionality) {
  // Test that all processors properly register and collect their internal states
  
  // AVX Processors
  auto avx_runsum = std::make_shared<AVXRunSumProcessor>();
  auto avx_abs_runsum = std::make_shared<AVXAbsRunSumProcessor>();
  auto avx_frugal = std::make_shared<AVXFrugalPedestalSubtractProcessor>();
  
  // Naive Processors  
  auto naive_runsum = std::make_shared<NaiveRunSumProcessor>();
  auto naive_abs_runsum = std::make_shared<NaiveAbsRunSumProcessor>();
  auto naive_frugal = std::make_shared<NaiveFrugalPedestalSubtractProcessor>();

  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  // Configure all processors with collection enabled
  auto config = create_test_config();
  config["memory_factor_plane0"] = 100;
  config["memory_factor_plane1"] = 200;
  config["memory_factor_plane2"] = 300;
  config["scale_factor_plane0"] = 10;
  config["scale_factor_plane1"] = 20;
  config["scale_factor_plane2"] = 30;
  config["memory_divisor_plane0"] = 10;
  config["memory_divisor_plane1"] = 10;
  config["memory_divisor_plane2"] = 10;
  config["scale_divisor_plane0"] = 10;
  config["scale_divisor_plane1"] = 10;
  config["scale_divisor_plane2"] = 10;
  config["accum_limit"] = 10;

  // Configure each processor with its specific states
  auto avx_runsum_config = config;
  avx_runsum_config["requested_internal_states"] = "r,s,rs";
  avx_runsum->configure(avx_runsum_config, plane_numbers);
  
  auto avx_abs_runsum_config = config;
  avx_abs_runsum_config["requested_internal_states"] = "r,s,rs";
  avx_abs_runsum->configure(avx_abs_runsum_config, plane_numbers);
  
  auto avx_frugal_config = config;
  avx_frugal_config["requested_internal_states"] = "pedestal,accum";
  avx_frugal->configure(avx_frugal_config, plane_numbers);
  
  auto naive_runsum_config = config;
  naive_runsum_config["requested_internal_states"] = "r,s,rs";
  naive_runsum->configure(naive_runsum_config, plane_numbers);
  
  auto naive_abs_runsum_config = config;
  naive_abs_runsum_config["requested_internal_states"] = "r,s,rs";
  naive_abs_runsum->configure(naive_abs_runsum_config, plane_numbers);
  
  auto naive_frugal_config = config;
  naive_frugal_config["requested_internal_states"] = "pedestal,accum";
  naive_frugal->configure(naive_frugal_config, plane_numbers);

  // Verify registration worked for all processors
  BOOST_TEST(avx_runsum->get_requested_internal_state_names().size() == 3);
  BOOST_TEST(avx_abs_runsum->get_requested_internal_state_names().size() == 3);
  BOOST_TEST(avx_frugal->get_requested_internal_state_names().size() == 2);
  BOOST_TEST(naive_runsum->get_requested_internal_state_names().size() == 3);
  BOOST_TEST(naive_abs_runsum->get_requested_internal_state_names().size() == 3);
  BOOST_TEST(naive_frugal->get_requested_internal_state_names().size() == 2);

  // Test collection works for all processors
  // Test AVX processors individually
  auto avx_runsum_bm = avx_runsum->_get_internal_state_buffer_manager();
  avx_runsum_bm->write_to_active_buffer();
  auto avx_runsum_state = avx_runsum_bm->switch_buffer_and_read_casted();
  BOOST_TEST(avx_runsum_state.m_data != nullptr);
  BOOST_TEST(avx_runsum_state.m_size > 0);

  auto avx_abs_runsum_bm = avx_abs_runsum->_get_internal_state_buffer_manager();
  avx_abs_runsum_bm->write_to_active_buffer();
  auto avx_abs_runsum_state = avx_abs_runsum_bm->switch_buffer_and_read_casted();
  BOOST_TEST(avx_abs_runsum_state.m_data != nullptr);
  BOOST_TEST(avx_abs_runsum_state.m_size > 0);

  auto avx_frugal_bm = avx_frugal->_get_internal_state_buffer_manager();
  avx_frugal_bm->write_to_active_buffer();
  auto avx_frugal_state = avx_frugal_bm->switch_buffer_and_read_casted();
  BOOST_TEST(avx_frugal_state.m_data != nullptr);
  BOOST_TEST(avx_frugal_state.m_size > 0);

  // Test Naive processors individually
  auto naive_runsum_bm = naive_runsum->_get_internal_state_buffer_manager();
  naive_runsum_bm->write_to_active_buffer();
  auto naive_runsum_state = naive_runsum_bm->switch_buffer_and_read_casted();
  BOOST_TEST(naive_runsum_state.m_data != nullptr);
  BOOST_TEST(naive_runsum_state.m_size > 0);

  auto naive_abs_runsum_bm = naive_abs_runsum->_get_internal_state_buffer_manager();
  naive_abs_runsum_bm->write_to_active_buffer();
  auto naive_abs_runsum_state = naive_abs_runsum_bm->switch_buffer_and_read_casted();
  BOOST_TEST(naive_abs_runsum_state.m_data != nullptr);
  BOOST_TEST(naive_abs_runsum_state.m_size > 0);

  auto naive_frugal_bm = naive_frugal->_get_internal_state_buffer_manager();
  naive_frugal_bm->write_to_active_buffer();
  auto naive_frugal_state = naive_frugal_bm->switch_buffer_and_read_casted();
  BOOST_TEST(naive_frugal_state.m_data != nullptr);
  BOOST_TEST(naive_frugal_state.m_size > 0);
}

BOOST_AUTO_TEST_CASE(test_processor_specific_internal_states) {
  // Verify each processor registers the correct internal state names
  
  auto avx_runsum = std::make_shared<AVXRunSumProcessor>();
  auto avx_frugal = std::make_shared<AVXFrugalPedestalSubtractProcessor>();
  auto naive_runsum = std::make_shared<NaiveRunSumProcessor>();
  auto naive_frugal = std::make_shared<NaiveFrugalPedestalSubtractProcessor>();

  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  auto config = create_test_config();
  config["memory_factor_plane0"] = 100;
  config["memory_factor_plane1"] = 200;
  config["memory_factor_plane2"] = 300;
  config["scale_factor_plane0"] = 10;
  config["scale_factor_plane1"] = 20;
  config["scale_factor_plane2"] = 30;
  config["memory_divisor_plane0"] = 10;
  config["memory_divisor_plane1"] = 10;
  config["memory_divisor_plane2"] = 10;
  config["scale_divisor_plane0"] = 10;
  config["scale_divisor_plane1"] = 10;
  config["scale_divisor_plane2"] = 10;
  config["accum_limit"] = 10;

  // Configure each processor with its specific states
  auto avx_runsum_config = config;
  avx_runsum_config["requested_internal_states"] = "r,s,rs";
  avx_runsum->configure(avx_runsum_config, plane_numbers);
  
  auto avx_frugal_config = config;
  avx_frugal_config["requested_internal_states"] = "pedestal,accum";
  avx_frugal->configure(avx_frugal_config, plane_numbers);
  
  auto naive_runsum_config = config;
  naive_runsum_config["requested_internal_states"] = "r,s,rs";
  naive_runsum->configure(naive_runsum_config, plane_numbers);
  
  auto naive_frugal_config = config;
  naive_frugal_config["requested_internal_states"] = "pedestal,accum";
  naive_frugal->configure(naive_frugal_config, plane_numbers);

  // Check AVXRunSumProcessor states
  auto avx_runsum_states = avx_runsum->get_requested_internal_state_names();
  BOOST_TEST(avx_runsum_states.size() == 3);
  BOOST_TEST((std::find(avx_runsum_states.begin(), avx_runsum_states.end(), "r") != avx_runsum_states.end()));
  BOOST_TEST((std::find(avx_runsum_states.begin(), avx_runsum_states.end(), "s") != avx_runsum_states.end()));
  BOOST_TEST((std::find(avx_runsum_states.begin(), avx_runsum_states.end(), "rs") != avx_runsum_states.end()));

  // Check AVXFrugalPedestalSubtractProcessor states
  auto avx_frugal_states = avx_frugal->get_requested_internal_state_names();
  BOOST_TEST(avx_frugal_states.size() == 2);
  BOOST_TEST((std::find(avx_frugal_states.begin(), avx_frugal_states.end(), "pedestal") != avx_frugal_states.end()));
  BOOST_TEST((std::find(avx_frugal_states.begin(), avx_frugal_states.end(), "accum") != avx_frugal_states.end()));

  // Check NaiveRunSumProcessor states
  auto naive_runsum_states = naive_runsum->get_requested_internal_state_names();
  BOOST_TEST(naive_runsum_states.size() == 3);
  BOOST_TEST((std::find(naive_runsum_states.begin(), naive_runsum_states.end(), "r") != naive_runsum_states.end()));
  BOOST_TEST((std::find(naive_runsum_states.begin(), naive_runsum_states.end(), "s") != naive_runsum_states.end()));
  BOOST_TEST((std::find(naive_runsum_states.begin(), naive_runsum_states.end(), "rs") != naive_runsum_states.end()));

  // Check NaiveFrugalPedestalSubtractProcessor states
  auto naive_frugal_states = naive_frugal->get_requested_internal_state_names();
  BOOST_TEST(naive_frugal_states.size() == 2);
  BOOST_TEST((std::find(naive_frugal_states.begin(), naive_frugal_states.end(), "pedestal") != naive_frugal_states.end()));
  BOOST_TEST((std::find(naive_frugal_states.begin(), naive_frugal_states.end(), "accum") != naive_frugal_states.end()));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tpglibs

#endif // TPGLIBS_ENABLE_STATE_MONITORING
