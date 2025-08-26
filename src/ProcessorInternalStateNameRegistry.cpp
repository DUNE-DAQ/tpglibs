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
    clear();
}

template <typename T>
ProcessorInternalStateNameRegistry<T>::~ProcessorInternalStateNameRegistry() {
    clear();
}

template <typename T>
size_t ProcessorInternalStateNameRegistry<T>::get_number_of_requested_internal_states() {
    return m_requested_internal_state_names.size();
}

template <typename T>
std::vector<std::string> ProcessorInternalStateNameRegistry<T>::get_names_of_requested_internal_states() {
    return m_requested_internal_state_names;
}

template <typename T>
void ProcessorInternalStateNameRegistry<T>::register_internal_state(std::string name, std::shared_ptr<signal_t> pointer_to_state) {
    m_internal_state_map[name] = pointer_to_state;
}

template <typename T>
std::shared_ptr<typename ProcessorInternalStateNameRegistry<T>::signal_t> 
ProcessorInternalStateNameRegistry<T>::get_internal_state_item_ptr(std::string name) {
    return m_internal_state_map[name];
}

template <typename T>
std::vector<std::string> ProcessorInternalStateNameRegistry<T>::get_all_registered_internal_state_names() {
    std::vector<std::string> names;
    for (const auto& item : m_internal_state_map) {
        names.push_back(item.first);
    }
    return names;
}

template <typename T>
void ProcessorInternalStateNameRegistry<T>::parse_requested_internal_state_items(std::string config_string) {
    // Clear existing requested names first
    m_requested_internal_state_names.clear();
    
    if (config_string.empty()) {
        return;
    }
    
    // Parse comma-separated items
    std::string::size_type start = 0;
    std::string::size_type end = config_string.find(',');
    
    while (end != std::string::npos) {
        // Extract item name and trim whitespace
        std::string item_name = config_string.substr(start, end - start);
        
        // Trim leading and trailing whitespace
        item_name.erase(0, item_name.find_first_not_of(" \t\r\n"));
        item_name.erase(item_name.find_last_not_of(" \t\r\n") + 1);
        
        // Only add non-empty item names
        if (!item_name.empty()) {
            m_requested_internal_state_names.push_back(item_name);
        }
        
        start = end + 1;
        end = config_string.find(',', start);
    }
    
    // Handle the last item (after the final comma or if there are no commas)
    if (start < config_string.length()) {
        std::string last_item = config_string.substr(start);
        
        // Trim leading and trailing whitespace
        last_item.erase(0, last_item.find_first_not_of(" \t\r\n"));
        last_item.erase(last_item.find_last_not_of(" \t\r\n") + 1);
        
        // Only add non-empty item names
        if (!last_item.empty()) {
            m_requested_internal_state_names.push_back(last_item);
        }
    }
}

template <typename T>
void ProcessorInternalStateNameRegistry<T>::clear() {
    // reinitialize member variables
    m_internal_state_map.clear();
    m_requested_internal_state_names.clear();
}

// Explicit template instantiations for common types
template class ProcessorInternalStateNameRegistry<__m256i>;
template class ProcessorInternalStateNameRegistry<std::array<int16_t, 16>>;

} // namespace tpglibs 