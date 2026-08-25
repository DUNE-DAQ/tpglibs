/**
 * @file ProcessorInternalStateBufferManager_test.cxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifdef TPGLIBS_ENABLE_STATE_MONITORING

#define BOOST_TEST_MODULE ProcessorInternalStateBufferManager_tests
#define FMT_HEADER_ONLY

#include "tpglibs/ProcessorInternalStateBufferManager.hpp"
#include "tpglibs/ProcessorInternalStateNameRegistry.hpp"

#include <boost/test/unit_test.hpp>
#include <immintrin.h>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>

namespace tpglibs {

// =============================================================================
// LIFECYCLE TESTS
// =============================================================================

BOOST_AUTO_TEST_SUITE(lifecycle_tests)

BOOST_AUTO_TEST_CASE(test_construction_and_destruction)
{
  // Test that we can construct and destroy without crashing
  ProcessorInternalStateBufferManager<__m256i> manager;
  BOOST_TEST(true); // If we reach here, construction succeeded
}

BOOST_AUTO_TEST_CASE(test_configure_single_item)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  __m256i a = _mm256_set1_epi16(42);
  auto a_ptr = std::make_shared<__m256i>(a);
  
  registry.register_internal_state("state_a", a_ptr);
  registry.parse_requested_internal_state_items("state_a");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  
  // Configuration should succeed without crash
  BOOST_TEST(true);
}

BOOST_AUTO_TEST_CASE(test_configure_multiple_items)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  __m256i a = _mm256_set1_epi16(1);
  __m256i b = _mm256_set1_epi16(2);
  __m256i c = _mm256_set1_epi16(3);
  
  auto a_ptr = std::make_shared<__m256i>(a);
  auto b_ptr = std::make_shared<__m256i>(b);
  auto c_ptr = std::make_shared<__m256i>(c);
  
  registry.register_internal_state("a", a_ptr);
  registry.register_internal_state("b", b_ptr);
  registry.register_internal_state("c", c_ptr);
  registry.parse_requested_internal_state_items("a,b,c");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  
  BOOST_TEST(true);
}

BOOST_AUTO_TEST_CASE(test_configure_empty_registry)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  // Don't register or request any items
  registry.parse_requested_internal_state_items("");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  
  // Should handle empty configuration gracefully
  BOOST_TEST(true);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// READ AND WRITE BEHAVIOR TESTS
// =============================================================================

BOOST_AUTO_TEST_SUITE(read_write_behavior_tests)

BOOST_AUTO_TEST_CASE(test_basic_write_and_read)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  __m256i a = _mm256_set1_epi16(100);
  auto a_ptr = std::make_shared<__m256i>(a);
  
  registry.register_internal_state("a", a_ptr);
  registry.parse_requested_internal_state_items("a");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  manager.write_to_active_buffer();
  
  auto result = manager.switch_buffer_and_read();
  
  BOOST_TEST(result.m_size == 1);
  BOOST_REQUIRE(result.m_data != nullptr);
  
  int16_t out[16];
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out), result.m_data[0]);
  
  for (int i = 0; i < 16; i++) {
    BOOST_TEST(out[i] == 100);
  }
}

BOOST_AUTO_TEST_CASE(test_multiple_items_write_and_read)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  __m256i a = _mm256_set1_epi16(10);
  __m256i b = _mm256_set1_epi16(20);
  __m256i c = _mm256_set1_epi16(30);
  
  auto a_ptr = std::make_shared<__m256i>(a);
  auto b_ptr = std::make_shared<__m256i>(b);
  auto c_ptr = std::make_shared<__m256i>(c);
  
  registry.register_internal_state("a", a_ptr);
  registry.register_internal_state("b", b_ptr);
  registry.register_internal_state("c", c_ptr);
  registry.parse_requested_internal_state_items("a,b,c");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  manager.write_to_active_buffer();
  
  auto result = manager.switch_buffer_and_read();
  
  BOOST_TEST(result.m_size == 3);
  BOOST_REQUIRE(result.m_data != nullptr);
  
  // Check each item
  int16_t expected_values[3] = {10, 20, 30};
  for (size_t item = 0; item < 3; ++item) {
    int16_t out[16];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out), result.m_data[item]);
    
    for (int i = 0; i < 16; i++) {
      BOOST_TEST(out[i] == expected_values[item]);
    }
  }
}

BOOST_AUTO_TEST_CASE(test_data_update_between_writes)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  __m256i state = _mm256_set1_epi16(100);
  auto state_ptr = std::make_shared<__m256i>(state);
  
  registry.register_internal_state("state", state_ptr);
  registry.parse_requested_internal_state_items("state");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  
  // First write
  manager.write_to_active_buffer();
  auto result1 = manager.switch_buffer_and_read();
  
  int16_t out1[16];
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out1), result1.m_data[0]);
  BOOST_TEST(out1[0] == 100);
  
  // Modify the state
  *state_ptr = _mm256_set1_epi16(200);
  
  // Second write
  manager.write_to_active_buffer();
  auto result2 = manager.switch_buffer_and_read();
  
  int16_t out2[16];
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out2), result2.m_data[0]);
  BOOST_TEST(out2[0] == 200);
}

BOOST_AUTO_TEST_CASE(test_multiple_consecutive_writes)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  __m256i state = _mm256_set1_epi16(1);
  auto state_ptr = std::make_shared<__m256i>(state);
  
  registry.register_internal_state("state", state_ptr);
  registry.parse_requested_internal_state_items("state");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  
  // Multiple writes without reads
  for (int i = 1; i <= 5; ++i) {
    *state_ptr = _mm256_set1_epi16(i * 10);
    manager.write_to_active_buffer();
  }
  
  // Read should get the last written value
  auto result = manager.switch_buffer_and_read();
  
  int16_t out[16];
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out), result.m_data[0]);
  BOOST_TEST(out[0] == 50); // Last value (5 * 10)
}

BOOST_AUTO_TEST_CASE(test_casted_read)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  __m256i a = _mm256_set1_epi16(123);
  auto a_ptr = std::make_shared<__m256i>(a);
  
  registry.register_internal_state("a", a_ptr);
  registry.parse_requested_internal_state_items("a");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  manager.write_to_active_buffer();
  
  auto result = manager.switch_buffer_and_read_casted();
  
  BOOST_TEST(result.m_size == 1);
  BOOST_REQUIRE(result.m_data != nullptr);
  
  // Check all 16 values in the array
  for (int i = 0; i < 16; i++) {
    BOOST_TEST(result.m_data[0][i] == 123);
  }
}

BOOST_AUTO_TEST_CASE(test_multiple_reads_without_write)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  __m256i state = _mm256_set1_epi16(777);
  auto state_ptr = std::make_shared<__m256i>(state);
  
  registry.register_internal_state("state", state_ptr);
  registry.parse_requested_internal_state_items("state");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  manager.write_to_active_buffer();
  
  // Multiple reads
  for (int i = 0; i < 3; ++i) {
    auto result = manager.switch_buffer_and_read();
    int16_t out[16];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out), result.m_data[0]);
    BOOST_TEST(out[0] == 777);
  }
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// THREAD SAFETY TESTS
// =============================================================================

BOOST_AUTO_TEST_SUITE(thread_safety_tests)

BOOST_AUTO_TEST_CASE(test_concurrent_single_writer_single_reader)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  __m256i state = _mm256_set1_epi16(0);
  auto state_ptr = std::make_shared<__m256i>(state);
  
  registry.register_internal_state("state", state_ptr);
  registry.parse_requested_internal_state_items("state");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  
  std::atomic<bool> done{false};
  std::atomic<int> write_count{0};
  std::atomic<int> read_count{0};
  
  // Writer thread
  std::thread writer([&]() {
    for (int i = 0; i < 1000; ++i) {
      *state_ptr = _mm256_set1_epi16(i);
      manager.write_to_active_buffer();
      write_count++;
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    done.store(true, std::memory_order_release);
  });
  
  // Reader thread
  std::thread reader([&]() {
    while (!done.load(std::memory_order_acquire)) {
      auto result = manager.switch_buffer_and_read();
      if (result.m_data != nullptr) {
        read_count++;
      }
      std::this_thread::sleep_for(std::chrono::microseconds(15));
    }
  });
  
  writer.join();
  reader.join();
  
  BOOST_TEST(write_count == 1000);
  BOOST_TEST(read_count > 0);
}

BOOST_AUTO_TEST_CASE(test_concurrent_single_writer_multiple_readers)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  __m256i state = _mm256_set1_epi16(0);
  auto state_ptr = std::make_shared<__m256i>(state);
  
  registry.register_internal_state("state", state_ptr);
  registry.parse_requested_internal_state_items("state");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  
  std::atomic<bool> done{false};
  std::atomic<int> total_reads{0};
  const int num_readers = 3;
  
  // Writer thread
  std::thread writer([&]() {
    for (int i = 0; i < 500; ++i) {
      *state_ptr = _mm256_set1_epi16(i);
      manager.write_to_active_buffer();
      std::this_thread::sleep_for(std::chrono::microseconds(20));
    }
    done.store(true, std::memory_order_release);
  });
  
  // Multiple reader threads
  std::vector<std::thread> readers;
  for (int r = 0; r < num_readers; ++r) {
    readers.emplace_back([&]() {
      int local_reads = 0;
      while (!done.load(std::memory_order_acquire)) {
        auto result = manager.switch_buffer_and_read();
        if (result.m_data != nullptr) {
          local_reads++;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(30));
      }
      total_reads.fetch_add(local_reads);
    });
  }
  
  writer.join();
  for (auto& reader : readers) {
    reader.join();
  }
  
  BOOST_TEST(total_reads > 0);
}

BOOST_AUTO_TEST_CASE(test_stress_many_operations)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  __m256i a = _mm256_set1_epi16(0);
  __m256i b = _mm256_set1_epi16(0);
  
  auto a_ptr = std::make_shared<__m256i>(a);
  auto b_ptr = std::make_shared<__m256i>(b);
  
  registry.register_internal_state("a", a_ptr);
  registry.register_internal_state("b", b_ptr);
  registry.parse_requested_internal_state_items("a,b");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  
  std::atomic<bool> done{false};
  
  // High-frequency writer
  std::thread writer([&]() {
    for (int i = 0; i < 10000; ++i) {
      *a_ptr = _mm256_set1_epi16(i % 1000);
      *b_ptr = _mm256_set1_epi16((i * 2) % 1000);
      manager.write_to_active_buffer();
    }
    done.store(true, std::memory_order_release);
  });
  
  // High-frequency reader
  std::thread reader([&]() {
    int successful_reads = 0;
    while (!done.load(std::memory_order_acquire)) {
      auto result = manager.switch_buffer_and_read();
      if (result.m_data != nullptr && result.m_size == 2) {
        successful_reads++;
      }
    }
    BOOST_TEST(successful_reads > 0);
  });
  
  writer.join();
  reader.join();
  
  BOOST_TEST(true); // Test passes if no crashes
}

BOOST_AUTO_TEST_CASE(test_casted_read_thread_safety)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  __m256i state = _mm256_set1_epi16(0);
  auto state_ptr = std::make_shared<__m256i>(state);
  
  registry.register_internal_state("state", state_ptr);
  registry.parse_requested_internal_state_items("state");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  
  std::atomic<bool> done{false};
  
  // Writer thread
  std::thread writer([&]() {
    for (int i = 0; i < 1000; ++i) {
      *state_ptr = _mm256_set1_epi16(i);
      manager.write_to_active_buffer();
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    done.store(true, std::memory_order_release);
  });
  
  // Reader using casted read
  std::thread reader([&]() {
    int successful_reads = 0;
    while (!done.load(std::memory_order_acquire)) {
      auto result = manager.switch_buffer_and_read_casted();
      if (result.m_data != nullptr) {
        successful_reads++;
      }
      std::this_thread::sleep_for(std::chrono::microseconds(15));
    }
    BOOST_TEST(successful_reads > 0);
  });
  
  writer.join();
  reader.join();
  
  BOOST_TEST(true);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// EDGE CASE TESTS
// =============================================================================

BOOST_AUTO_TEST_SUITE(edge_case_tests)

BOOST_AUTO_TEST_CASE(test_read_before_any_write)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  __m256i state = _mm256_set1_epi16(555);
  auto state_ptr = std::make_shared<__m256i>(state);
  
  registry.register_internal_state("state", state_ptr);
  registry.parse_requested_internal_state_items("state");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  
  // Read without writing first - should not crash
  auto result = manager.switch_buffer_and_read();
  BOOST_TEST(result.m_data != nullptr);
}

BOOST_AUTO_TEST_CASE(test_alternating_write_read_pattern)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  __m256i state = _mm256_set1_epi16(0);
  auto state_ptr = std::make_shared<__m256i>(state);
  
  registry.register_internal_state("state", state_ptr);
  registry.parse_requested_internal_state_items("state");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  
  // Alternating write-read pattern
  for (int i = 0; i < 10; ++i) {
    *state_ptr = _mm256_set1_epi16(i * 10);
    manager.write_to_active_buffer();
    
    auto result = manager.switch_buffer_and_read();
    int16_t out[16];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out), result.m_data[0]);
    BOOST_TEST(out[0] == i * 10);
  }
}

BOOST_AUTO_TEST_CASE(test_rapid_buffer_switching)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  __m256i state = _mm256_set1_epi16(999);
  auto state_ptr = std::make_shared<__m256i>(state);
  
  registry.register_internal_state("state", state_ptr);
  registry.parse_requested_internal_state_items("state");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  manager.write_to_active_buffer();
  
  // Rapid consecutive reads (buffer switches)
  for (int i = 0; i < 100; ++i) {
    auto result = manager.switch_buffer_and_read();
    BOOST_TEST(result.m_data != nullptr);
    BOOST_TEST(result.m_size == 1);
  }
}

BOOST_AUTO_TEST_CASE(test_partial_item_selection)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  __m256i a = _mm256_set1_epi16(10);
  __m256i b = _mm256_set1_epi16(20);
  __m256i c = _mm256_set1_epi16(30);
  
  auto a_ptr = std::make_shared<__m256i>(a);
  auto b_ptr = std::make_shared<__m256i>(b);
  auto c_ptr = std::make_shared<__m256i>(c);
  
  registry.register_internal_state("a", a_ptr);
  registry.register_internal_state("b", b_ptr);
  registry.register_internal_state("c", c_ptr);
  
  // Only request a subset of registered items
  registry.parse_requested_internal_state_items("a,c");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  manager.write_to_active_buffer();
  
  auto result = manager.switch_buffer_and_read();
  
  // Should only have 2 items (a and c)
  BOOST_TEST(result.m_size == 2);
  
  int16_t out0[16], out1[16];
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out0), result.m_data[0]);
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out1), result.m_data[1]);
  
  BOOST_TEST(out0[0] == 10); // a
  BOOST_TEST(out1[0] == 30); // c
}

BOOST_AUTO_TEST_CASE(test_large_number_of_items)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  
  const int num_items = 20;
  std::vector<std::shared_ptr<__m256i>> ptrs;
  std::string request_string;
  
  for (int i = 0; i < num_items; ++i) {
    __m256i val = _mm256_set1_epi16(i * 100);
    auto ptr = std::make_shared<__m256i>(val);
    ptrs.push_back(ptr);
    
    std::string name = "state_" + std::to_string(i);
    registry.register_internal_state(name, ptr);
    
    if (i > 0) request_string += ",";
    request_string += name;
  }
  
  registry.parse_requested_internal_state_items(request_string);
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  manager.write_to_active_buffer();
  
  auto result = manager.switch_buffer_and_read();
  
  BOOST_TEST(result.m_size == num_items);
  
  // Verify each item
  for (int i = 0; i < num_items; ++i) {
    int16_t out[16];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out), result.m_data[i]);
    BOOST_TEST(out[0] == i * 100);
  }
}

BOOST_AUTO_TEST_CASE(test_data_consistency_after_multiple_switches)
{
  ProcessorInternalStateNameRegistry<__m256i> registry;
  __m256i state = _mm256_set1_epi16(12345);
  auto state_ptr = std::make_shared<__m256i>(state);
  
  registry.register_internal_state("state", state_ptr);
  registry.parse_requested_internal_state_items("state");
  
  ProcessorInternalStateBufferManager<__m256i> manager;
  manager.configure_from_registry(&registry);
  
  // Write once
  manager.write_to_active_buffer();
  
  // Multiple reads - data should remain consistent
  for (int i = 0; i < 5; ++i) {
    auto result = manager.switch_buffer_and_read();
    int16_t out[16];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out), result.m_data[0]);
    BOOST_TEST(out[0] == 12345);
  }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tpglibs
 

#endif // TPGLIBS_ENABLE_STATE_MONITORING
