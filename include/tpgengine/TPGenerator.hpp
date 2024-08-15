/**
 * @file TPGenerator.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGENGINE_TPGENERATOR_HPP_
#define TPGENGINE_TPGENERATOR_HPP_

#include "tpgengine/AVXPipeline.hpp"

#include <utility>

namespace tpgengine {

class TPGenerator {
  static const uint8_t m_num_channels_per_pipeline = 16; // AVX2 with int16 data samples allows us to process 16 channels.
  uint8_t m_num_pipelines = 0;  // Gets set inside configure.
  std::vector<AVXPipeline> m_tpg_pipelines;
  int m_sample_tick_difference;

  public:
    void configure(const std::vector<nlohmann::json>& configs,
                   const std::vector<std::pair<int16_t, int16_t>> channel_plane_numbers,
                   const int sample_tick_difference);

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
          __m256i regi = _mm256_lddqu_si256((__m256i*)cursor);
          __m256i expanded_subframe = expand_frame(regi);
          std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps = m_tpg_pipelines[p].process(expanded_subframe);

          // Need to insert all the TPs while scaling and shifting time_start and time_peak.
          for (auto tp : tps) {
            tp.time_start = (t - tp.time_over_threshold) * m_sample_tick_difference + timestamp;
            tp.time_peak  = tp.time_peak * m_sample_tick_difference + tp.time_start;
            tp_aggr.push_back(tp);
          }
          cursor += register_alignment / 8; // Numerator is in bits. Need bytes.
        }
      }

      return tp_aggr;
    }

  private:
    __m256i expand_frame(const __m256i& regi);
};

} // namespace tpgengine

#endif // TPGENGINE_TPGENERATOR_HPP_
