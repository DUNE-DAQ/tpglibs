/**
 * @file ProcessorInternalStateBufferManager_test.cxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

 #define BOOST_TEST_MODULE boost_test_macro_overview
 #define FMT_HEADER_ONLY
 
 #include "tpglibs/ProcessorInternalStateBufferManager.hpp"
 #include "tpglibs/ProcessorInternalStateNameRegistry.hpp"
 
 #include <boost/test/unit_test.hpp>
 #include <immintrin.h>
 
 namespace tpglibs {
 
 BOOST_AUTO_TEST_CASE(test_processor_internal_state_buffer_manager_sanity)
 {
  ProcessorInternalStateNameRegistry<__m256i> registry;

  __m256i a = _mm256_set1_epi16(1);
  auto a_ptr = std::make_shared<__m256i>(a);

  registry.register_internal_state("a", a_ptr);
  registry.parse_requested_internal_state_items("a");

  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  manager.write_to_active_buffer();

  auto b = manager.switch_buffer_and_read();

  BOOST_TEST(b.m_size == 1);
  
  int16_t out0[16];
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out0), b.m_data[0]);

  for (size_t i = 0; i < 16; i++) {
    BOOST_TEST(out0[i] == 1);
  }
  
 }
 } // namespace tpglibs
 