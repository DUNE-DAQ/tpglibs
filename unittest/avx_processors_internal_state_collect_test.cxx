/**
 * @file avx_processors_internal_state_collect_test.cxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE boost_test_macro_overview
#define FMT_HEADER_ONLY

#include "tpglibs/AVXFrugalPedestalSubtractProcessor.hpp"

#include <boost/test/unit_test.hpp>
#include <immintrin.h>
#include <atomic>

namespace tpglibs {

BOOST_AUTO_TEST_CASE(test_avx_internal_state_collect) {
  std::shared_ptr<AVXProcessor> pc = std::make_shared<AVXFrugalPedestalSubtractProcessor>();

  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};

  nlohmann::json pc_config = {
    {"accum_limit", 42},
    {"metric_collect_toggle_state", true}
  };

  pc->configure(pc_config, plane_numbers);

  auto buffer_manager = pc->_get_internal_state_buffer_manager();
  auto registry = pc->_get_internal_state_name_registry();
  registry->parse_requested_internal_state_items("pedestal");

  buffer_manager->write_to_active_buffer();

  auto state_value = buffer_manager->switch_buffer_and_read_casted();

  BOOST_TEST(state_value.m_size == 1);
  BOOST_TEST(state_value.m_data != nullptr);

  for (size_t i = 0; i < 16; i++) {
    BOOST_TEST(state_value.m_data[0][i] == 16384); // set to 0x4000 initially
  }
}

BOOST_AUTO_TEST_CASE(test_avx_metric_multithread) {
  // Set up processor and config
  auto pc = std::make_shared<AVXFrugalPedestalSubtractProcessor>();
  int16_t plane_numbers[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};
  nlohmann::json pc_config = {{"accum_limit", 42}, {"metric_collect_toggle_state", true}};
  pc->configure(pc_config, plane_numbers);

  // Launch producer thread
  std::atomic<bool> done{false};
  auto buffer_manager = pc->_get_internal_state_buffer_manager();
  auto registry = pc->_get_internal_state_name_registry();
  registry->parse_requested_internal_state_items("pedestal");
  std::thread producer([&](){
    for (int i = 0; i < 10000; ++i) {
      buffer_manager->write_to_active_buffer();
    }
    done.store(true, std::memory_order_release);
  });

  // Launch consumer thread
  std::thread consumer([&](){
    while (!done.load(std::memory_order_acquire)) {
      auto metric = buffer_manager->switch_buffer_and_read_casted();
      BOOST_TEST(metric.m_data != nullptr);
      BOOST_TEST(metric.m_size == 1);
      for (size_t i = 0; i < 16; i++) {
        BOOST_TEST(metric.m_data[0][i] == 16384); // set to 0x4000 initially
      }
    }
  });

  producer.join();
  consumer.join();
}

} // namespace tpglibs