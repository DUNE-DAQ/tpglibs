/**
 * @file ProcessorInternalStateNameRegistry.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/ProcessorInternalStateNameRegistry.hpp"
#include <immintrin.h>
#include <array>

namespace tpglibs {

template <typename T>
ProcessorInternalStateNameRegistry<T>::ProcessorInternalStateNameRegistry() {
    // Basic stub - do nothing
}

template <typename T>
ProcessorInternalStateNameRegistry<T>::~ProcessorInternalStateNameRegistry() {
    // Basic stub - do nothing
}

template <typename T>
size_t ProcessorInternalStateNameRegistry<T>::get_number_of_requested_internal_states() {
    // Basic stub - return 0
    return 0;
}

template <typename T>
std::vector<std::string> ProcessorInternalStateNameRegistry<T>::get_names_of_requested_internal_states() {
    // Basic stub - return empty vector
    return {};
}

template <typename T>
void ProcessorInternalStateNameRegistry<T>::register_internal_state(std::string name, std::shared_ptr<signal_t> pointer_to_state) {
    // Basic stub - do nothing
}

template <typename T>
std::shared_ptr<typename ProcessorInternalStateNameRegistry<T>::signal_t> 
ProcessorInternalStateNameRegistry<T>::get_internal_state_item_ptr(std::string name) {
    // Basic stub - return nullptr
    return nullptr;
}

template <typename T>
std::vector<std::string> ProcessorInternalStateNameRegistry<T>::get_all_registered_internal_state_names() {
    // Basic stub - return empty vector
    return {};
}

template <typename T>
void ProcessorInternalStateNameRegistry<T>::parse_requested_internal_state_items(std::string config_string) {
    // Basic stub - do nothing
}

template <typename T>
void ProcessorInternalStateNameRegistry<T>::clear() {
    // Basic stub - do nothing
}

// Explicit template instantiations for common types
template class ProcessorInternalStateNameRegistry<__m256i>;
template class ProcessorInternalStateNameRegistry<std::array<int16_t, 16>>;

} // namespace tpglibs 