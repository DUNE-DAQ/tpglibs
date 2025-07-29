/**
 * @file TPGenerator.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_TPGENERATOR_HPP_
#define TPGLIBS_TPGENERATOR_HPP_

#include <memory>
#include "tpglibs/ProcessorMetricCollector.hpp"
#include "tpglibs/AVXPipeline.hpp"

#include "trgdataformats/Types.hpp"

#include <utility>
#include <memory>
#include <unordered_map>


namespace tpglibs {

/**
 * @class TPGenerator
 *
 * @brief TPG driving class.
 *
 * This is the interface that receives raw data frames to process and outputs the TPs accordingly.
 */
class TPGenerator {
  static const uint8_t m_num_channels_per_pipeline = 16; // AVX2 with int16 data samples allows us to process 16 channels.
  uint8_t m_num_pipelines = 0;  // Gets set inside configure.
  std::vector<AVXPipeline> m_tpg_pipelines;
  int m_sample_tick_difference;
  std::vector<uint16_t> m_sot_minima{1,1,1};  // Defaults to 1 for all planes.
  void* m_processor_metric_collector = nullptr;
  bool m_tpg_metric_collect_enabled;

  public:
    /**
     * @brief Setup and configure the AVX pipelines.
     *
     * @param configs A vector of pairs: AVX pipeline to use and its configuration.
     * @param channel_plane_numbers A vector of channel numbers and their plane numbers.
     * @param sample_tick_difference Number of ticks between time samples in expected data frames.
     */
    void configure(const std::vector<std::pair<std::string, nlohmann::json>>& configs,
                   const std::vector<std::pair<dunedaq::trgdataformats::channel_t, int16_t>> channel_plane_numbers,
                   const int sample_tick_difference);

    /**
     * @brief Set the minimum samples over threshold for a TP according to plane.
     *
     * @param sot_minima TPs from plane `i` will have at least `sot_minima[i]` value for its samples_over_threshold.
     */
    void set_sot_minima(const std::vector<uint16_t>& sot_minima);

    /**
     * @brief Lazily initialize and return the processor metric collector.
     *
     * If the collector has not yet been created, allocate a new
     * ProcessorMetricCollector<__m256i> instance and store it in
     * `m_processor_metric_collector`.
     *
     * @return void* Pointer to the processor metric collector instance.
     */
    void* get_processor_metric_collector();

    void set_metric_collector_enable_state(bool state) {
      m_tpg_metric_collect_enabled = state;
    }

    void signal_metric_collection();

    std::unordered_map<dunedaq::trgdataformats::channel_t, std::vector<std::pair<std::string, int16_t>>> get_processor_metrics();

    void free_metric_collector() {
      if (m_processor_metric_collector != nullptr) {
        auto collector = static_cast<ProcessorMetricCollector<__m256i>*>(m_processor_metric_collector);
        
        collector->stop();
        
        delete collector;
        
        m_processor_metric_collector = nullptr;
      }
    }

    /**
     * @brief Driving function for the TPG.
     *
     * This function receives the frames, expands, sends down AVX pipelines, and returns TPs that were generated.
     *
     * @param frame A data frame to process and generate TPs from.
     * @return A vector of TPs.
     */
    template <typename T>
    std::vector<dunedaq::trgdataformats::TriggerPrimitive> operator()(const T* frame) {
      // Max number of TPs for a channel: number of time samples / 2.
      std::vector<dunedaq::trgdataformats::TriggerPrimitive> tp_aggr;
      tp_aggr.reserve(T::s_num_channels * T::s_time_samples_per_frame / 2);

      const typename T::word_t (*words_ptr)[T::s_bits_per_adc] = frame->adc_words;
      const uint64_t timestamp = frame->get_timestamp();

      const int register_alignment = T::s_bits_per_adc * m_num_channels_per_pipeline;
      // Loop in time.
      for (int t = 0; t < T::s_time_samples_per_frame; t++) {
        const typename T::word_t *time_sample = *(words_ptr + t);
        char* cursor = (char*) time_sample; // Need to walk in terms of bytes/bits.

        // Loop in pipelines.
        for (int p = 0; p < m_num_pipelines; p++) {
          if (p == m_num_pipelines - 1)
            cursor -= 4; // Take a step of 32 bit backwards for the last sub-frame.

          __m256i regi = _mm256_lddqu_si256((__m256i*)cursor);

          if (p == m_num_pipelines - 1) // Permute the row order to use the same operation.
            regi = _mm256_permutevar8x32_epi32(regi, _mm256_setr_epi32(1, 2, 3, 4, 5, 6, 7, 0));

          __m256i expanded_subframe = expand_frame(regi);
          std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps = m_tpg_pipelines[p].process(expanded_subframe);

          for (auto tp : tps) {
            tp.time_start = (t - tp.samples_over_threshold) * m_sample_tick_difference + timestamp;
            tp_aggr.push_back(tp);
          }
          cursor += register_alignment / 8; // Numerator is in bits. Need bytes.
        }
      }

      return tp_aggr;
    }

  private:
    __m256i expand_frame(const __m256i& regi); /// @brief Expansion from 14-bit signals to 16-bit.
    __m256i old_expand_frame(const __m256i& regi); /// @brief Legacy expansion function.
};

} // namespace tpglibs

#endif // TPGLIBS_TPGENERATOR_HPP_
