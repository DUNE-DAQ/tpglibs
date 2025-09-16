/**
 * @file ProcessorInternalStateBufferManager.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/ProcessorInternalStateBufferManager.hpp"
#include <immintrin.h>
#include <array>
#include <cstdint>
#include <mm_malloc.h>


namespace tpglibs {

template <typename T>
ProcessorInternalStateBufferManager<T>::ProcessorInternalStateBufferManager() {
}

template <typename T>
ProcessorInternalStateBufferManager<T>::~ProcessorInternalStateBufferManager() {
    clear();
}

template <typename T>
void ProcessorInternalStateBufferManager<T>::configure_from_registry(std::shared_ptr<ProcessorInternalStateNameRegistry<signal_t>> registry) {
    // obtain the number of internal state items
    auto num_items = registry->get_number_of_requested_internal_states();
    allocate_buffers(num_items);
    // reset active buffer ptr to first buffer
    m_active_buffer.store(&m_store_buffers[0], std::memory_order_release);
    // obtain the pointers to the internal state items
    m_internal_state_item_ptrs = registry->get_all_requested_internal_state_item_ptrs();
}

template <typename T>
void ProcessorInternalStateBufferManager<T>::allocate_buffers(size_t buffer_size) {
    for (auto& buf : m_store_buffers) {
        buf.m_size = buffer_size;
        buf.m_data = static_cast<T*>(
            _mm_malloc(buf.m_size * sizeof(T), alignof(T))
        );
    }
}

template <typename T>
void ProcessorInternalStateBufferManager<T>::switch_active_buffer() {
    auto current_active = m_active_buffer.load(std::memory_order_acquire);
    auto new_active = (current_active == &m_store_buffers[0])
                        ? &m_store_buffers[1]
                        : &m_store_buffers[0];
    m_active_buffer.store(new_active, std::memory_order_release);
}

template <typename T>
void ProcessorInternalStateBufferManager<T>::write_to_active_buffer() {
  m_seq.fetch_add(1, std::memory_order_release);
  auto free_ptr = m_active_buffer.load(std::memory_order_acquire);
  // write to free buffer
  
  for (size_t i = 0; i < m_internal_state_item_ptrs.size(); i++) {
    free_ptr->m_data[i] = *m_internal_state_item_ptrs[i];
  }

  //set seq to indicate write end
  m_seq.fetch_add(1, std::memory_order_release);
}

template <typename T>
ProcessorMetricArray<typename ProcessorInternalStateBufferManager<T>::signal_t> ProcessorInternalStateBufferManager<T>::switch_buffer_and_read() {
    // Basic stub - return default constructed object
    uint16_t start_seq;

    do {
        start_seq = m_seq.load(std::memory_order_acquire);
    } while (start_seq & 1); // spin if writer is mid-write

    auto current_active = m_active_buffer.load(std::memory_order_acquire);
    switch_active_buffer();

    return *current_active;
}

template <typename T>
void ProcessorInternalStateBufferManager<T>::clear() {
    for (auto& buf : m_store_buffers) {
        _mm_free(buf.m_data);
    }
}

// Explicit template instantiations for common types
template class ProcessorInternalStateBufferManager<__m256i>;
template class ProcessorInternalStateBufferManager<std::array<int16_t, 16>>;

} // namespace tpglibs 