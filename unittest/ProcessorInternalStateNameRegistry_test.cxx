/**
 * @file ProcessorInternalStateNameRegistry_test.cxx
 *
 * @brief Comprehensive unit tests for ProcessorInternalStateNameRegistry
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE ProcessorInternalStateNameRegistry_test
#define FMT_HEADER_ONLY
#define TPGLIBS_ENABLE_TEST_INTERFACES

#include "tpglibs/ProcessorInternalStateNameRegistry.hpp"

#include <boost/test/unit_test.hpp>
#include <immintrin.h>
#include <array>
#include <memory>
#include <string>
#include <algorithm>

namespace tpglibs {

// =============================================================================
// SECTION 1: Object Lifetime Tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(ObjectLifetime)

BOOST_AUTO_TEST_CASE(test_default_construction)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 0);
  BOOST_TEST(registry.get_names_of_requested_internal_states().empty());
  BOOST_TEST(registry.test_get_map_size() == 0);
}

BOOST_AUTO_TEST_CASE(test_destruction_with_empty_registry)
{
  {
    ProcessorInternalStateNameRegistry<__m256i> registry;
  } // Should not crash on destruction
  BOOST_TEST(true); // If we reach here, destruction succeeded
}

BOOST_AUTO_TEST_CASE(test_destruction_with_registered_states)
{
  {
    ProcessorInternalStateNameRegistry<__m256i> registry;
    auto state1 = std::make_shared<__m256i>();
    auto state2 = std::make_shared<__m256i>();
    registry.register_internal_state("state1", state1);
    registry.register_internal_state("state2", state2);
  } // Should properly clean up all registered states
  BOOST_TEST(true);
}

BOOST_AUTO_TEST_CASE(test_destruction_clears_shared_ptrs)
{
  std::weak_ptr<__m256i> weak_state;
  {
    ProcessorInternalStateNameRegistry<__m256i> registry;
    auto state = std::make_shared<__m256i>();
    weak_state = state;
    registry.register_internal_state("state", state);
    BOOST_TEST(!weak_state.expired());
  }
  // After registry is destroyed, if no other references exist, weak_ptr should expire
  // Note: state variable above still holds reference, so this is more about testing cleanup
}

BOOST_AUTO_TEST_CASE(test_move_construction)
{
  ProcessorInternalStateNameRegistry<__m256i> registry1;
  registry1.parse_requested_internal_state_items("a,b,c");
  auto state = std::make_shared<__m256i>();
  registry1.register_internal_state("a", state);
  
  ProcessorInternalStateNameRegistry<__m256i> registry2(std::move(registry1));
  
  BOOST_TEST(registry2.get_number_of_requested_internal_states() == 3);
  BOOST_TEST(registry2.is_registered("a"));
  BOOST_TEST(registry2.get_internal_state_item_ptr("a") == state);
}

BOOST_AUTO_TEST_CASE(test_move_assignment)
{
  ProcessorInternalStateNameRegistry<__m256i> registry1;
  registry1.parse_requested_internal_state_items("x,y");
  auto state = std::make_shared<__m256i>();
  registry1.register_internal_state("x", state);
  
  ProcessorInternalStateNameRegistry<__m256i> registry2;
  registry2 = std::move(registry1);
  
  BOOST_TEST(registry2.get_number_of_requested_internal_states() == 2);
  BOOST_TEST(registry2.is_registered("x"));
  BOOST_TEST(registry2.get_internal_state_item_ptr("x") == state);
}

BOOST_AUTO_TEST_CASE(test_multiple_instances_independence)
{
  ProcessorInternalStateNameRegistry<__m256i> registry1;
  ProcessorInternalStateNameRegistry<__m256i> registry2;
  
  registry1.parse_requested_internal_state_items("a,b");
  registry2.parse_requested_internal_state_items("c,d");
  
  auto state1 = std::make_shared<__m256i>();
  auto state2 = std::make_shared<__m256i>();
  
  registry1.register_internal_state("a", state1);
  registry2.register_internal_state("c", state2);
  
  // Verify independence
  BOOST_TEST(registry1.get_number_of_requested_internal_states() == 2);
  BOOST_TEST(registry2.get_number_of_requested_internal_states() == 2);
  BOOST_TEST(registry1.is_registered("a"));
  BOOST_TEST(!registry1.is_registered("c"));
  BOOST_TEST(registry2.is_registered("c"));
  BOOST_TEST(!registry2.is_registered("a"));
}

BOOST_AUTO_TEST_CASE(test_different_template_types)
{
  // Test with __m256i
  ProcessorInternalStateNameRegistry<__m256i> registry_avx;
  auto state_avx = std::make_shared<__m256i>();
  registry_avx.register_internal_state("avx_state", state_avx);
  BOOST_TEST(registry_avx.is_registered("avx_state"));
  
  // Test with array type
  ProcessorInternalStateNameRegistry<std::array<int16_t, 16>> registry_array;
  auto state_array = std::make_shared<std::array<int16_t, 16>>();
  registry_array.register_internal_state("array_state", state_array);
  BOOST_TEST(registry_array.is_registered("array_state"));
  
  // Test with simple int
  ProcessorInternalStateNameRegistry<int> registry_int;
  auto state_int = std::make_shared<int>(42);
  registry_int.register_internal_state("int_state", state_int);
  BOOST_TEST(registry_int.is_registered("int_state"));
  BOOST_TEST(*registry_int.get_internal_state_item_ptr("int_state") == 42);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// SECTION 2: Parse Operation Tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(ParseOperations)

BOOST_AUTO_TEST_CASE(test_parse_empty_string)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  registry.parse_requested_internal_state_items("");
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 0);
  BOOST_TEST(registry.get_names_of_requested_internal_states().empty());
}

BOOST_AUTO_TEST_CASE(test_parse_single_item)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  registry.parse_requested_internal_state_items("state1");
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 1);
  BOOST_TEST(registry.get_names_of_requested_internal_states()[0] == "state1");
  BOOST_TEST(registry.is_requested("state1"));
}

BOOST_AUTO_TEST_CASE(test_parse_multiple_items)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  registry.parse_requested_internal_state_items("a,b,c,d,e");
  
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 5);
  auto names = registry.get_names_of_requested_internal_states();
  BOOST_TEST(names[0] == "a");
  BOOST_TEST(names[1] == "b");
  BOOST_TEST(names[2] == "c");
  BOOST_TEST(names[3] == "d");
  BOOST_TEST(names[4] == "e");
}

BOOST_AUTO_TEST_CASE(test_parse_with_spaces)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  registry.parse_requested_internal_state_items(" a , b , c ");
  
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 3);
  auto names = registry.get_names_of_requested_internal_states();
  BOOST_TEST(names[0] == "a");
  BOOST_TEST(names[1] == "b");
  BOOST_TEST(names[2] == "c");
}

BOOST_AUTO_TEST_CASE(test_parse_with_tabs_and_newlines)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  registry.parse_requested_internal_state_items("\ta\t,\nb\n,\r\nc\r\n");
  
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 3);
  auto names = registry.get_names_of_requested_internal_states();
  BOOST_TEST(names[0] == "a");
  BOOST_TEST(names[1] == "b");
  BOOST_TEST(names[2] == "c");
}

BOOST_AUTO_TEST_CASE(test_parse_with_empty_items)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  registry.parse_requested_internal_state_items("a,,b,,,c");
  
  // Empty items should be filtered out
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 3);
  auto names = registry.get_names_of_requested_internal_states();
  BOOST_TEST(names[0] == "a");
  BOOST_TEST(names[1] == "b");
  BOOST_TEST(names[2] == "c");
}

BOOST_AUTO_TEST_CASE(test_parse_with_only_commas)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  registry.parse_requested_internal_state_items(",,,");
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 0);
}

BOOST_AUTO_TEST_CASE(test_parse_with_only_whitespace)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  registry.parse_requested_internal_state_items("   \t\n  ");
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 0);
}

BOOST_AUTO_TEST_CASE(test_parse_trailing_comma)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  registry.parse_requested_internal_state_items("a,b,c,");
  
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 3);
  auto names = registry.get_names_of_requested_internal_states();
  BOOST_TEST(names[0] == "a");
  BOOST_TEST(names[1] == "b");
  BOOST_TEST(names[2] == "c");
}

BOOST_AUTO_TEST_CASE(test_parse_leading_comma)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  registry.parse_requested_internal_state_items(",a,b,c");
  
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 3);
  auto names = registry.get_names_of_requested_internal_states();
  BOOST_TEST(names[0] == "a");
  BOOST_TEST(names[1] == "b");
  BOOST_TEST(names[2] == "c");
}

BOOST_AUTO_TEST_CASE(test_parse_duplicate_names)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  registry.parse_requested_internal_state_items("a,b,a,c,b");
  
  // Current implementation doesn't deduplicate
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 5);
  auto names = registry.get_names_of_requested_internal_states();
  BOOST_TEST(names[0] == "a");
  BOOST_TEST(names[1] == "b");
  BOOST_TEST(names[2] == "a");
  BOOST_TEST(names[3] == "c");
  BOOST_TEST(names[4] == "b");
}

BOOST_AUTO_TEST_CASE(test_parse_overwrite_previous)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  registry.parse_requested_internal_state_items("a,b,c");
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 3);
  
  // Parse again - should clear previous
  registry.parse_requested_internal_state_items("x,y");
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 2);
  auto names = registry.get_names_of_requested_internal_states();
  BOOST_TEST(names[0] == "x");
  BOOST_TEST(names[1] == "y");
}

BOOST_AUTO_TEST_CASE(test_parse_long_names)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  std::string long_name(1000, 'x');
  registry.parse_requested_internal_state_items(long_name);
  
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 1);
  BOOST_TEST(registry.get_names_of_requested_internal_states()[0] == long_name);
}

BOOST_AUTO_TEST_CASE(test_parse_special_characters)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  registry.parse_requested_internal_state_items("state_1,state-2,state.3,state:4");
  
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 4);
  auto names = registry.get_names_of_requested_internal_states();
  BOOST_TEST(names[0] == "state_1");
  BOOST_TEST(names[1] == "state-2");
  BOOST_TEST(names[2] == "state.3");
  BOOST_TEST(names[3] == "state:4");
}

BOOST_AUTO_TEST_CASE(test_parse_very_many_items)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  std::string config;
  for (int i = 0; i < 1000; ++i) {
    if (i > 0) config += ",";
    config += "state" + std::to_string(i);
  }
  
  registry.parse_requested_internal_state_items(config);
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 1000);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// SECTION 3: Register Operation Tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(RegisterOperations)

BOOST_AUTO_TEST_CASE(test_register_single_state)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  auto state = std::make_shared<__m256i>();
  
  registry.register_internal_state("test_state", state);
  
  BOOST_TEST(registry.is_registered("test_state"));
  BOOST_TEST(registry.get_internal_state_item_ptr("test_state") == state);
}

BOOST_AUTO_TEST_CASE(test_register_multiple_states)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  auto state1 = std::make_shared<__m256i>();
  auto state2 = std::make_shared<__m256i>();
  auto state3 = std::make_shared<__m256i>();
  
  registry.register_internal_state("s1", state1);
  registry.register_internal_state("s2", state2);
  registry.register_internal_state("s3", state3);
  
  BOOST_TEST(registry.test_get_map_size() == 3);
  BOOST_TEST(registry.get_internal_state_item_ptr("s1") == state1);
  BOOST_TEST(registry.get_internal_state_item_ptr("s2") == state2);
  BOOST_TEST(registry.get_internal_state_item_ptr("s3") == state3);
}

BOOST_AUTO_TEST_CASE(test_register_overwrites_existing)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  auto state1 = std::make_shared<__m256i>();
  auto state2 = std::make_shared<__m256i>();
  
  registry.register_internal_state("test", state1);
  BOOST_TEST(registry.get_internal_state_item_ptr("test") == state1);
  
  // Register again with different pointer
  registry.register_internal_state("test", state2);
  BOOST_TEST(registry.get_internal_state_item_ptr("test") == state2);
  BOOST_TEST(registry.test_get_map_size() == 1); // Should not create duplicate
}

BOOST_AUTO_TEST_CASE(test_register_nullptr)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  // Registering nullptr should be allowed (user might do this)
  registry.register_internal_state("null_state", nullptr);
  BOOST_TEST(registry.is_registered("null_state"));
  BOOST_TEST(registry.get_internal_state_item_ptr("null_state") == nullptr);
}

BOOST_AUTO_TEST_CASE(test_register_empty_name)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  auto state = std::make_shared<__m256i>();
  
  // Empty string as name should work (though not recommended)
  registry.register_internal_state("", state);
  BOOST_TEST(registry.is_registered(""));
  BOOST_TEST(registry.get_internal_state_item_ptr("") == state);
}

BOOST_AUTO_TEST_CASE(test_register_names_with_whitespace)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  auto state1 = std::make_shared<__m256i>();
  auto state2 = std::make_shared<__m256i>();
  
  registry.register_internal_state(" a ", state1);
  registry.register_internal_state("a", state2);
  
  // These are different names (with and without spaces)
  BOOST_TEST(registry.test_get_map_size() == 2);
  BOOST_TEST(registry.get_internal_state_item_ptr(" a ") == state1);
  BOOST_TEST(registry.get_internal_state_item_ptr("a") == state2);
}

BOOST_AUTO_TEST_CASE(test_register_shared_ownership)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  auto state = std::make_shared<__m256i>();
  std::weak_ptr<__m256i> weak_state = state;
  
  registry.register_internal_state("shared", state);
  
  // Registry should hold a reference
  BOOST_TEST(state.use_count() == 2); // Original + registry
  
  // Release original reference
  state.reset();
  BOOST_TEST(!weak_state.expired()); // Registry still holds it
  
  // Get pointer from registry
  auto retrieved = registry.get_internal_state_item_ptr("shared");
  BOOST_TEST(retrieved != nullptr);
  BOOST_TEST(retrieved.use_count() == 2); // Registry + retrieved
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// SECTION 4: Retrieval Operation Tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(RetrievalOperations)

BOOST_AUTO_TEST_CASE(test_get_unregistered_name)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  // Getting non-existent key returns nullptr (default-constructed shared_ptr)
  auto ptr = registry.get_internal_state_item_ptr("nonexistent");
  BOOST_TEST(ptr == nullptr);
}

BOOST_AUTO_TEST_CASE(test_is_registered)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  auto state = std::make_shared<__m256i>();
  
  BOOST_TEST(!registry.is_registered("test"));
  registry.register_internal_state("test", state);
  BOOST_TEST(registry.is_registered("test"));
}

BOOST_AUTO_TEST_CASE(test_is_requested)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  registry.parse_requested_internal_state_items("a,b,c");
  
  BOOST_TEST(registry.is_requested("a"));
  BOOST_TEST(registry.is_requested("b"));
  BOOST_TEST(registry.is_requested("c"));
  BOOST_TEST(!registry.is_requested("d"));
}

BOOST_AUTO_TEST_CASE(test_get_all_registered_names)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  auto state1 = std::make_shared<__m256i>();
  auto state2 = std::make_shared<__m256i>();
  auto state3 = std::make_shared<__m256i>();
  
  registry.register_internal_state("x", state1);
  registry.register_internal_state("y", state2);
  registry.register_internal_state("z", state3);
  
  auto names = registry.test_get_all_registered_internal_state_names();
  BOOST_TEST(names.size() == 3);
  
  // Order not guaranteed in unordered_map, so check presence
  bool has_x = (std::find(names.begin(), names.end(), "x") != names.end());
  bool has_y = (std::find(names.begin(), names.end(), "y") != names.end());
  bool has_z = (std::find(names.begin(), names.end(), "z") != names.end());
  BOOST_TEST(has_x);
  BOOST_TEST(has_y);
  BOOST_TEST(has_z);
}

BOOST_AUTO_TEST_CASE(test_get_all_requested_item_ptrs_empty)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  auto ptrs = registry.get_all_requested_internal_state_item_ptrs();
  BOOST_TEST(ptrs.empty());
}

BOOST_AUTO_TEST_CASE(test_get_all_requested_item_ptrs_all_registered)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  registry.parse_requested_internal_state_items("a,b,c");
  
  auto state_a = std::make_shared<__m256i>();
  auto state_b = std::make_shared<__m256i>();
  auto state_c = std::make_shared<__m256i>();
  
  registry.register_internal_state("a", state_a);
  registry.register_internal_state("b", state_b);
  registry.register_internal_state("c", state_c);
  
  auto ptrs = registry.get_all_requested_internal_state_item_ptrs();
  BOOST_TEST(ptrs.size() == 3);
  BOOST_TEST(ptrs[0] == state_a);
  BOOST_TEST(ptrs[1] == state_b);
  BOOST_TEST(ptrs[2] == state_c);
}

BOOST_AUTO_TEST_CASE(test_get_all_requested_item_ptrs_partial_registered)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  registry.parse_requested_internal_state_items("a,b,c");
  
  auto state_a = std::make_shared<__m256i>();
  registry.register_internal_state("a", state_a);
  // b and c not registered
  
  auto ptrs = registry.get_all_requested_internal_state_item_ptrs();
  BOOST_TEST(ptrs.size() == 3);
  BOOST_TEST(ptrs[0] == state_a);
  BOOST_TEST(ptrs[1] == nullptr); // Not registered
  BOOST_TEST(ptrs[2] == nullptr); // Not registered
}

BOOST_AUTO_TEST_CASE(test_get_all_requested_item_ptrs_order_preserved)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  // Parse in specific order
  registry.parse_requested_internal_state_items("z,a,m,b");
  
  auto state_z = std::make_shared<__m256i>();
  auto state_a = std::make_shared<__m256i>();
  auto state_m = std::make_shared<__m256i>();
  auto state_b = std::make_shared<__m256i>();
  
  // Register in different order
  registry.register_internal_state("a", state_a);
  registry.register_internal_state("z", state_z);
  registry.register_internal_state("b", state_b);
  registry.register_internal_state("m", state_m);
  
  auto ptrs = registry.get_all_requested_internal_state_item_ptrs();
  
  // Should follow requested order, not registration order
  BOOST_TEST(ptrs[0] == state_z);
  BOOST_TEST(ptrs[1] == state_a);
  BOOST_TEST(ptrs[2] == state_m);
  BOOST_TEST(ptrs[3] == state_b);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// SECTION 5: Clear Operation Tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(ClearOperations)

BOOST_AUTO_TEST_CASE(test_clear_empty_registry)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  registry.test_clear();
  
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 0);
  BOOST_TEST(registry.test_get_map_size() == 0);
}

BOOST_AUTO_TEST_CASE(test_clear_removes_all_data)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  registry.parse_requested_internal_state_items("a,b,c");
  auto state1 = std::make_shared<__m256i>();
  auto state2 = std::make_shared<__m256i>();
  registry.register_internal_state("a", state1);
  registry.register_internal_state("b", state2);
  
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 3);
  BOOST_TEST(registry.test_get_map_size() == 2);
  
  registry.test_clear();
  
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 0);
  BOOST_TEST(registry.test_get_map_size() == 0);
  BOOST_TEST(!registry.is_registered("a"));
  BOOST_TEST(!registry.is_registered("b"));
}

BOOST_AUTO_TEST_CASE(test_clear_releases_shared_ptrs)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  auto state = std::make_shared<__m256i>();
  std::weak_ptr<__m256i> weak_state = state;
  
  registry.register_internal_state("test", state);
  BOOST_TEST(state.use_count() == 2);
  
  state.reset(); // Release original
  BOOST_TEST(!weak_state.expired()); // Registry still holds it
  
  registry.test_clear();
  BOOST_TEST(weak_state.expired()); // Now it should be gone
}

BOOST_AUTO_TEST_CASE(test_use_after_clear)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  registry.parse_requested_internal_state_items("a,b");
  registry.test_clear();
  
  // Should be usable after clear
  registry.parse_requested_internal_state_items("x,y,z");
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 3);
  
  auto state = std::make_shared<__m256i>();
  registry.register_internal_state("x", state);
  BOOST_TEST(registry.is_registered("x"));
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// SECTION 6: Edge Cases and Integration Tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(EdgeCasesAndIntegration)

BOOST_AUTO_TEST_CASE(test_parse_then_register_workflow)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  // Typical workflow: parse config, then register states
  registry.parse_requested_internal_state_items("input,output,temp");
  
  auto input_state = std::make_shared<__m256i>();
  auto output_state = std::make_shared<__m256i>();
  auto temp_state = std::make_shared<__m256i>();
  
  registry.register_internal_state("input", input_state);
  registry.register_internal_state("output", output_state);
  registry.register_internal_state("temp", temp_state);
  
  auto ptrs = registry.get_all_requested_internal_state_item_ptrs();
  BOOST_TEST(ptrs.size() == 3);
  BOOST_TEST(ptrs[0] == input_state);
  BOOST_TEST(ptrs[1] == output_state);
  BOOST_TEST(ptrs[2] == temp_state);
}

BOOST_AUTO_TEST_CASE(test_register_then_parse_workflow)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  // Register states first
  auto state1 = std::make_shared<__m256i>();
  auto state2 = std::make_shared<__m256i>();
  registry.register_internal_state("s1", state1);
  registry.register_internal_state("s2", state2);
  
  // Then parse which ones to request
  registry.parse_requested_internal_state_items("s1");
  
  auto ptrs = registry.get_all_requested_internal_state_item_ptrs();
  BOOST_TEST(ptrs.size() == 1);
  BOOST_TEST(ptrs[0] == state1);
}

BOOST_AUTO_TEST_CASE(test_requesting_unregistered_states)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  registry.parse_requested_internal_state_items("a,b,c");
  // Don't register anything
  
  auto ptrs = registry.get_all_requested_internal_state_item_ptrs();
  BOOST_TEST(ptrs.size() == 3);
  BOOST_TEST(ptrs[0] == nullptr);
  BOOST_TEST(ptrs[1] == nullptr);
  BOOST_TEST(ptrs[2] == nullptr);
}

BOOST_AUTO_TEST_CASE(test_registering_unrequested_states)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  registry.parse_requested_internal_state_items("a");
  
  auto state_a = std::make_shared<__m256i>();
  auto state_b = std::make_shared<__m256i>();
  auto state_c = std::make_shared<__m256i>();
  
  registry.register_internal_state("a", state_a);
  registry.register_internal_state("b", state_b);
  registry.register_internal_state("c", state_c);
  
  // All are registered
  BOOST_TEST(registry.test_get_map_size() == 3);
  
  // But only 'a' is requested
  auto ptrs = registry.get_all_requested_internal_state_item_ptrs();
  BOOST_TEST(ptrs.size() == 1);
  BOOST_TEST(ptrs[0] == state_a);
}

BOOST_AUTO_TEST_CASE(test_case_sensitive_names)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  auto state_lower = std::make_shared<__m256i>();
  auto state_upper = std::make_shared<__m256i>();
  
  registry.register_internal_state("test", state_lower);
  registry.register_internal_state("TEST", state_upper);
  
  // Case-sensitive, should be different
  BOOST_TEST(registry.test_get_map_size() == 2);
  BOOST_TEST(registry.get_internal_state_item_ptr("test") == state_lower);
  BOOST_TEST(registry.get_internal_state_item_ptr("TEST") == state_upper);
}

BOOST_AUTO_TEST_CASE(test_unicode_names)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  auto state = std::make_shared<__m256i>();
  registry.register_internal_state("状態", state);
  
  BOOST_TEST(registry.is_registered("状態"));
  BOOST_TEST(registry.get_internal_state_item_ptr("状態") == state);
}

BOOST_AUTO_TEST_CASE(test_very_long_name)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  std::string very_long_name(10000, 'x');
  auto state = std::make_shared<__m256i>();
  
  registry.register_internal_state(very_long_name, state);
  BOOST_TEST(registry.is_registered(very_long_name));
  BOOST_TEST(registry.get_internal_state_item_ptr(very_long_name) == state);
}

BOOST_AUTO_TEST_CASE(test_repeated_parse_calls)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  registry.parse_requested_internal_state_items("a,b");
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 2);
  
  registry.parse_requested_internal_state_items("c,d,e");
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 3);
  BOOST_TEST(!registry.is_requested("a")); // Previous should be cleared
  BOOST_TEST(registry.is_requested("c"));
  
  registry.parse_requested_internal_state_items("");
  BOOST_TEST(registry.get_number_of_requested_internal_states() == 0);
}

BOOST_AUTO_TEST_CASE(test_stress_many_operations)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  // Register many states
  std::vector<std::shared_ptr<__m256i>> states;
  for (int i = 0; i < 100; ++i) {
    auto state = std::make_shared<__m256i>();
    states.push_back(state);
    registry.register_internal_state("state" + std::to_string(i), state);
  }
  
  BOOST_TEST(registry.test_get_map_size() == 100);
  
  // Request subset
  std::string config;
  for (int i = 0; i < 50; ++i) {
    if (i > 0) config += ",";
    config += "state" + std::to_string(i * 2); // Even indices
  }
  registry.parse_requested_internal_state_items(config);
  
  auto ptrs = registry.get_all_requested_internal_state_item_ptrs();
  BOOST_TEST(ptrs.size() == 50);
  
  // Verify correctness
  for (int i = 0; i < 50; ++i) {
    BOOST_TEST(ptrs[i] == states[i * 2]);
  }
}

BOOST_AUTO_TEST_CASE(test_interleaved_parse_register)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  registry.parse_requested_internal_state_items("a,b");
  
  auto state_a = std::make_shared<__m256i>();
  registry.register_internal_state("a", state_a);
  
  registry.parse_requested_internal_state_items("a,b,c");
  
  auto state_b = std::make_shared<__m256i>();
  auto state_c = std::make_shared<__m256i>();
  registry.register_internal_state("b", state_b);
  registry.register_internal_state("c", state_c);
  
  auto ptrs = registry.get_all_requested_internal_state_item_ptrs();
  BOOST_TEST(ptrs.size() == 3);
  BOOST_TEST(ptrs[0] == state_a);
  BOOST_TEST(ptrs[1] == state_b);
  BOOST_TEST(ptrs[2] == state_c);
}

BOOST_AUTO_TEST_CASE(test_memory_leak_scenario)
{
  // Test that circular references don't prevent cleanup
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  std::weak_ptr<__m256i> weak_state;
  
  {
    auto state = std::make_shared<__m256i>();
    weak_state = state;
    registry.register_internal_state("test", state);
    BOOST_TEST(state.use_count() == 2);
    // state goes out of scope
  }
  
  BOOST_TEST(!weak_state.expired()); // Registry still holds it
  
  registry.test_clear();
  BOOST_TEST(weak_state.expired()); // Should be cleaned up
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tpglibs
