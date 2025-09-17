/**
 * @file ProcessorInternalStateNameRegistry.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

 #include <string>
 #include <vector>
 #include <memory>
 #include <unordered_map>
 #include <immintrin.h>
 #include <array>

 #ifndef TPGLIBS_PROCESSORINTERNALSTATENAMEREGISTRY_HPP_
 #define TPGLIBS_PROCESSORINTERNALSTATENAMEREGISTRY_HPP_
 
 namespace tpglibs {
 
  /**
   * @class ProcessorInternalStateNameRegistry
   *
   * @brief Registry of internal state names.
   */
  template <typename T>
  class ProcessorInternalStateNameRegistry {
    public:
      /** @brief Signal type to use. Generally __m256i or std::array<int16_t, 16>; */
      using signal_t = T;

      /** @brief Constructor. */
      ProcessorInternalStateNameRegistry();

      /** @brief Destructor. */
      ~ProcessorInternalStateNameRegistry();

      /** @brief Get the number of requested internal states.
       *
       *  @return The number of requested internal states.
      */
      size_t get_number_of_requested_internal_states();

      /** @brief Get the names of the requested internal states.
       *
       *  @return The names of the requested internal states.
      */
      std::vector<std::string> get_names_of_requested_internal_states();

      /** @brief Parse the requested internal state items from a configuration string.
       *
       *  @param config_string The configuration string.
      */
      void parse_requested_internal_state_items(std::string config_string);

      /** @brief Register an internal state.
       *
       *  @param name The name of the internal state.
       *  @param pointer_to_state A pointer to the internal state item.
      */
      void register_internal_state(std::string name, std::shared_ptr<signal_t> pointer_to_state);

      /** @brief Get a pointer to an internal state item.
       *
       *  @param name The name of the internal state.
       *  @return A pointer to the internal state item.
      */
      std::shared_ptr<signal_t> get_internal_state_item_ptr(std::string name);

      /** @brief Get a vector of pointers to all internal state items.
       *
       *  @return A vector of pointers to all internal state items.
      */
      std::vector<std::shared_ptr<signal_t>> get_all_requested_internal_state_item_ptrs();

    protected:

      /** @brief Get all registered internal state names.
       *
       *  @return A vector of all registered internal state names.
      */
      std::vector<std::string> get_all_registered_internal_state_names();

      /** @brief Clear the registry. */
      void clear();

    private:
      /** @brief Map of internal state names to pointers. */
      std::unordered_map<std::string, std::shared_ptr<signal_t>> m_internal_state_map;

      /** @brief Vector of all requested internal state names. */
      std::vector<std::string> m_requested_internal_state_names;
  };

    // Template function implementations
    template <typename T>
    ProcessorInternalStateNameRegistry<T>::ProcessorInternalStateNameRegistry() {
        // clear();
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

    template <typename T>
    std::vector<std::shared_ptr<typename ProcessorInternalStateNameRegistry<T>::signal_t>> 
    ProcessorInternalStateNameRegistry<T>::get_all_requested_internal_state_item_ptrs() {
        std::vector<std::shared_ptr<typename ProcessorInternalStateNameRegistry<T>::signal_t>> item_ptrs;
        for (const auto& item : m_requested_internal_state_names) {
            item_ptrs.push_back(m_internal_state_map[item]);
        }
        return item_ptrs;
    }

} // namespace tpglibs
 
#endif // TPGLIBS_PROCESSORINTERNALSTATENAMEREGISTRY_HPP_
 