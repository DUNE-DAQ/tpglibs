/**
 * @file ProcessorInternalStateNameRegistry_test.cxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

 #define BOOST_TEST_MODULE boost_test_macro_overview
 #define FMT_HEADER_ONLY
 
 #include "tpglibs/ProcessorInternalStateNameRegistry.hpp"
 
 #include <boost/test/unit_test.hpp>
 #include <immintrin.h>
 
 namespace tpglibs {
 
 BOOST_AUTO_TEST_CASE(test_processor_internal_state_name_registry_sanity)
 {
  ProcessorInternalStateNameRegistry<__m256i> registry;
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 0);
  BOOST_TEST(registry.get_names_of_requested_internal_states().empty());
 }

 BOOST_AUTO_TEST_CASE(test_processor_internal_state_name_registry_register_parse)
 {
  ProcessorInternalStateNameRegistry<__m256i> registry;
  registry.parse_requested_internal_state_items("a,b,c");
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 3);
  BOOST_TEST(registry.get_names_of_requested_internal_states()[0] == "a");
  BOOST_TEST(registry.get_names_of_requested_internal_states()[1] == "b");
  BOOST_TEST(registry.get_names_of_requested_internal_states()[2] == "c");
 }
 } // namespace tpglibs
 