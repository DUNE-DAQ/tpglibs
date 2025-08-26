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

    protected:

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
} // namespace tpglibs
 
 #endif // TPGLIBS_PROCESSORINTERNALSTATENAMEREGISTRY_HPP_
 