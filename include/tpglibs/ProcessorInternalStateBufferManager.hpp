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
      void configure_from_registry(std::shared_ptr<ProcessorInternalStateNameRegistry<signal_t>> registry);

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

  } // namespace tpglibs
 
 #endif // TPGLIBS_PROCESSORINTERNALSTATEBUFFERMANAGER_HPP_
 