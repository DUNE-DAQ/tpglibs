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

namespace tpglibs {

template <typename T>
ProcessorInternalStateBufferManager<T>::ProcessorInternalStateBufferManager(size_t num_buffers, size_t buffer_size) 
    : m_buffer_size(buffer_size) {
    // Basic stub - do nothing
}

template <typename T>
ProcessorInternalStateBufferManager<T>::~ProcessorInternalStateBufferManager() {
    // Basic stub - do nothing
}

template <typename T>
void ProcessorInternalStateBufferManager<T>::configure_from_registry(ProcessorInternalStateNameRegistry<signal_t>& registry) {
    // Basic stub - do nothing
}

template <typename T>
void ProcessorInternalStateBufferManager<T>::allocate_buffers(size_t buffer_size) {
    // Basic stub - do nothing
}

template <typename T>
void ProcessorInternalStateBufferManager<T>::switch_active_buffer() {
    // Basic stub - do nothing
}

template <typename T>
void ProcessorInternalStateBufferManager<T>::write_to_active_buffer(std::vector<std::shared_ptr<signal_t>> data) {
    // Basic stub - do nothing
}

template <typename T>
ProcessorMetricArray<typename ProcessorInternalStateBufferManager<T>::signal_t> ProcessorInternalStateBufferManager<T>::read_from_inactive_buffer() {
    // Basic stub - return default constructed object
    return {};
}

template <typename T>
void ProcessorInternalStateBufferManager<T>::clear() {
    // Basic stub - do nothing
}

// Explicit template instantiations for common types
template class ProcessorInternalStateBufferManager<__m256i>;
template class ProcessorInternalStateBufferManager<std::array<int16_t, 16>>;

} // namespace tpglibs 