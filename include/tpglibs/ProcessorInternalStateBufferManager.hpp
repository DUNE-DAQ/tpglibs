/**
 * @file ProcessorInternalStateBufferManager.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

 #include "tpglibs/ProcessorInternalStateNameRegistry.hpp"
 #include "tpglibs/ProcessorMetricArray.hpp"

 #include <string>
 #include <vector>
 #include <memory>
 #include <atomic>
 #include <immintrin.h>
 #include <array>
 #include <cstdint>
 #include <mm_malloc.h>

 #ifndef TPGLIBS_PROCESSORINTERNALSTATEBUFFERMANAGER_HPP_
 #define TPGLIBS_PROCESSORINTERNALSTATEBUFFERMANAGER_HPP_
 
 namespace tpglibs {
 
  /**
   * @class ProcessorInternalStateBufferManager
   *
   * @brief Manages the internal state storage buffers for a processor.
   */
  template <typename T>
  class ProcessorInternalStateBufferManager {
    public:
      /** @brief Signal type to use. Generally __m256i or std::array<int16_t, 16>; */
      using signal_t = T;

      /** @brief Constructor. */
      ProcessorInternalStateBufferManager();

      /** @brief Destructor. */
      ~ProcessorInternalStateBufferManager();

      /** @brief Write to the active buffer.
       *
       *  @param data The data to write.
      */
      void write_to_active_buffer();

      /** @brief Read from the inactive buffer.
       *
       *  @return The data read from the inactive buffer.
      */
      ProcessorMetricArray<signal_t> switch_buffer_and_read();

      /** @brief Read from the inactive buffer and cast to std::array<int16_t, 16>. */
      ProcessorMetricArray<std::array<int16_t, 16>> switch_buffer_and_read_casted();

      /** @brief Configure and allocate correct buffer storage given the configuration string.
       *
       *  @param registry The registry object of internal state names.
      */
      void configure_from_registry(ProcessorInternalStateNameRegistry<signal_t>* registry);

      /** @brief clear all buffers and deallocate memory. */
      void clear();

    protected:

      /** @brief Allocate the correct size for the double buffer read and write buffers.
       *
       *  @param buffer_size The size of the buffer.
      */
      void allocate_buffers(size_t buffer_size);

      /** @brief Switch the active buffer. */
      void switch_active_buffer();

    private:
      /** @brief The vector of pointers to the internal state items. */
      std::vector<std::shared_ptr<signal_t>> m_internal_state_item_ptrs;

      /** @brief The double buffers for storing the internal state data. */
      ProcessorMetricArray<signal_t> m_store_buffers[2]{};

      /** @brief The active buffer. */
      std::atomic<ProcessorMetricArray<signal_t>*> m_active_buffer = &m_store_buffers[0];

      /** @brief The sequence number. */
      std::atomic<uint16_t> m_seq{0};

      /** @brief size of each buffer. */
      size_t m_buffer_size;
    };

    // Template function implementations
    template <typename T>
    ProcessorInternalStateBufferManager<T>::ProcessorInternalStateBufferManager() {
    }

    template <typename T>
    ProcessorInternalStateBufferManager<T>::~ProcessorInternalStateBufferManager() {
        clear();
    }

    template <typename T>
    void ProcessorInternalStateBufferManager<T>::configure_from_registry(ProcessorInternalStateNameRegistry<signal_t>* registry) {
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

    // Template specializations
    // Specialization for __m256i -> std::array<int16_t, 16> cast
    template <>
    inline ProcessorMetricArray<std::array<int16_t, 16>> ProcessorInternalStateBufferManager<__m256i>::switch_buffer_and_read_casted() {
        // First, get the raw data using switch_buffer_and_read
        auto raw_data = switch_buffer_and_read();
        
        std::vector<std::array<int16_t, 16>> temp_buffer;
        temp_buffer.resize(raw_data.m_size);
        
        // Cast each __m256i to std::array<int16_t, 16>
        for (size_t i = 0; i < raw_data.m_size; ++i) {
            // Extract 16 int16_t values from __m256i
            __m256i source = raw_data.m_data[i];
            
            // Use _mm256_storeu_si256 to store the data, then copy to array
            alignas(32) int16_t temp[16];
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(temp), source);
            
            // Copy to the result array
            for (int j = 0; j < 16; ++j) {
                temp_buffer[i][j] = temp[j];
            }
        }

        // Create result with pointer to the vector data
        ProcessorMetricArray<std::array<int16_t, 16>> result;
        result.m_size = raw_data.m_size;
        result.m_data = temp_buffer.data();

        return result;
    }

    // Specialization for std::array<int16_t, 16> -> std::array<int16_t, 16> cast (trivial)
    template <>
    inline ProcessorMetricArray<std::array<int16_t, 16>> ProcessorInternalStateBufferManager<std::array<int16_t, 16>>::switch_buffer_and_read_casted() {
        // First, get the raw data using switch_buffer_and_read
        auto raw_data = switch_buffer_and_read();
        
        // For std::array<int16_t, 16>, the cast is trivial - just return the data as-is
        return raw_data;
    }

  } // namespace tpglibs
 
 #endif // TPGLIBS_PROCESSORINTERNALSTATEBUFFERMANAGER_HPP_
 