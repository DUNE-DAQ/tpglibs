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
        void clear() {
            for (auto& buf : m_store_buffers) { _mm_free(buf.m_data); }
        }
        
        protected:
        
        /** @brief Allocate the correct size for the double buffer read and write buffers.
        *
        *  @param buffer_size The size of the buffer.
        */
        void allocate_buffers(size_t buffer_size);
        
        /** @brief Allocate the correct size for the double buffer read and write buffers for the casted data.
        *
        *  @param buffer_size The size of the buffer.
        */
        void allocate_cast_buffers(size_t) {}; // Do nothing for the generic template
        
        /** @brief Switch the active buffer. */
        void switch_active_buffer();
        
        private:
        /** @brief The vector of pointers to the internal state items. */
        std::vector<std::shared_ptr<signal_t>> m_internal_state_item_ptrs;
        
        /** @brief The double buffers for storing the internal state data. */
        ProcessorMetricArray<signal_t> m_store_buffers[2]{};
        
        /** @brief The double buffers for storing the internal state data casted to std::array<int16_t, 16>. */
        ProcessorMetricArray<std::array<int16_t,16>> m_cast_store_buffers[2]{};
        
        /** @brief The active buffer for the casted data. */
        std::atomic<ProcessorMetricArray<std::array<int16_t,16>>*> m_cast_active_buffer{ &m_cast_store_buffers[0] };
        
        /** @brief The write buffer pointer (buffer writer currently uses). */
        std::atomic<ProcessorMetricArray<signal_t>*> m_write_buffer = &m_store_buffers[0];
        
        /** @brief The read buffer pointer (buffer reader currently uses). */
        std::atomic<ProcessorMetricArray<signal_t>*> m_read_buffer = &m_store_buffers[0];
        
        /** @brief The sequence number for writes (odd=writing, even=complete). */
        std::atomic<uint16_t> m_write_seq{0};
        
        /** @brief The last sequence number that was read. */
        std::atomic<uint16_t> m_last_read_seq{0};
        
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
        allocate_cast_buffers(num_items);
        // Writer starts with buffer 0, reader starts with buffer 1 (they must be different!)
        m_write_buffer.store(&m_store_buffers[0], std::memory_order_release);
        m_read_buffer.store(&m_store_buffers[1], std::memory_order_release);
        m_cast_active_buffer.store(&m_cast_store_buffers[0], std::memory_order_release);
        // reset sequence counters
        m_write_seq.store(0, std::memory_order_release);
        m_last_read_seq.store(0, std::memory_order_release);
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
    void ProcessorInternalStateBufferManager<T>::write_to_active_buffer() {
        // Increment seq to indicate write start (becomes odd)
        m_write_seq.fetch_add(1, std::memory_order_release);
        
        auto write_ptr = m_write_buffer.load(std::memory_order_acquire);
        
        // Write to write buffer
        for (size_t i = 0; i < m_internal_state_item_ptrs.size(); i++) {
            // Check for nullptr before dereferencing (handles invalid state names)
            if (m_internal_state_item_ptrs[i] != nullptr) {
                write_ptr->m_data[i] = *m_internal_state_item_ptrs[i];
            } else {
                // If pointer is null, write a zeroed value
                write_ptr->m_data[i] = T{};
            }
        }
        
        // Increment seq to indicate write complete (becomes even)
        m_write_seq.fetch_add(1, std::memory_order_release);
    }
    
    template <typename T>
    ProcessorMetricArray<typename ProcessorInternalStateBufferManager<T>::signal_t> ProcessorInternalStateBufferManager<T>::switch_buffer_and_read() {
        // Wait until no write is in progress
        uint16_t current_write_seq;
        do {
            current_write_seq = m_write_seq.load(std::memory_order_acquire);
        } while (current_write_seq & 1); // spin if writer is mid-write (odd seq number)
        
        // Check if there's new data since last read
        uint16_t last_read = m_last_read_seq.load(std::memory_order_acquire);
        
        if (current_write_seq != last_read && current_write_seq > 0) {
            // New data available - swap the buffers between reader and writer
            auto current_read = m_read_buffer.load(std::memory_order_acquire);
            auto current_write = m_write_buffer.load(std::memory_order_acquire);
            
            // Swap: reader gets what writer just finished, writer gets what reader was using
            m_read_buffer.store(current_write, std::memory_order_release);
            m_write_buffer.store(current_read, std::memory_order_release);
            
            // Update last read sequence
            m_last_read_seq.store(current_write_seq, std::memory_order_release);
        }
        
        // Return data from read buffer
        auto read_ptr = m_read_buffer.load(std::memory_order_acquire);
        return *read_ptr;
    }
    
    // Template specializations
    // Specialization for __m256i since it has additional cast buffers.
    template<>
    inline void ProcessorInternalStateBufferManager<__m256i>::clear() {
        // free double buffer for temp values
        for (auto& buf : m_store_buffers) { _mm_free(buf.m_data); }
        // free double buffer for casted values
        for (auto& buf : m_cast_store_buffers) { _mm_free(buf.m_data); }
    }
    
    // Specialization for __m256i -> std::array<int16_t, 16> cast
    template <>
    inline ProcessorMetricArray<std::array<int16_t, 16>> ProcessorInternalStateBufferManager<__m256i>::switch_buffer_and_read_casted() {
        // First, get the raw data using switch_buffer_and_read
        auto raw_data = switch_buffer_and_read();
        
        auto* cast_free = m_cast_active_buffer.load(std::memory_order_acquire);
        
        // Cast each __m256i to std::array<int16_t, 16>
        for (size_t i = 0; i < raw_data.m_size; ++i) {
            // Cast and save to cast buffer
            _mm256_store_si256(
                reinterpret_cast<__m256i*>(cast_free->m_data[i].data()),
                raw_data.m_data[i]
            );
        }
        
        // Flip the active cast buffer
        
        auto* next = (cast_free == &m_cast_store_buffers[0]) ? &m_cast_store_buffers[1] : &m_cast_store_buffers[0];
        m_cast_active_buffer.store(next, std::memory_order_release);
        
        return *cast_free;
    }
    
    // Specialization for __m256i
    template<>
    inline void ProcessorInternalStateBufferManager<__m256i>::allocate_cast_buffers(size_t n) {
        for (auto& buf : m_cast_store_buffers) {
            buf.m_size = n;
            buf.m_data = static_cast<std::array<int16_t,16>*>(
                _mm_malloc(n * sizeof(std::array<int16_t,16>), 32)
            );
        }
        m_cast_active_buffer.store(&m_cast_store_buffers[0], std::memory_order_release);
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
