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
#include "fddetdataformats/WIBEthFrame.hpp"

namespace tpgengine {

//template <typename T>
class TPGenerator {
  using T = dunedaq::fddetdataformats::WIBEthFrame;
  static const uint8_t m_num_channels_per_pipeline = 16; // AVX2 with int16 data samples allows us to process 16 channels.
  static const uint8_t m_num_channels_per_frame = 64;//T::s_channels_per_half_femb;
  static const uint8_t m_num_pipelines = 4;//m_num_channels_per_frame / m_num_channels_per_pipeline;
  AVXPipeline m_tpg_pipelines[m_num_pipelines];

  public:
    void configure(const std::vector<nlohmann::json>& configs);
    void set_plane_numbers(const int16_t* plane_numbers);

    std::vector<dunedaq::trgdataformats::TriggerPrimitive> operator()(const T* frame) {
      // Max number of TPs for a channel: number of time samples / 2.
      std::vector<dunedaq::trgdataformats::TriggerPrimitive> tp_aggr;
      tp_aggr.reserve(T::s_num_channels * T::s_time_samples_per_frame / 2);

      const T::word_t (*words_ptr)[T::s_bits_per_adc] = frame->adc_words;

      const int register_alignment = T::s_bits_per_adc * m_num_channels_per_pipeline;
      // Loop in time.
      for (int t = 0; t < T::s_time_samples_per_frame; t++) {
        const T::word_t *time_sample = *(words_ptr + t);
        char* cursor = (char*) time_sample; // Need to walk in terms of bytes/bits.

        // Loop in pipelines.
        for (int p = 0; p < m_num_pipelines; p++) {
          __m256i regi = _mm256_lddqu_si256((__m256i*)cursor);
          __m256i expanded_subframe = expand_frame(regi);
          std::vector<dunedaq::trgdataformats::TriggerPrimitive> tps = m_tpg_pipelines[p].process(expanded_subframe);
          if (!tps.empty())
            tp_aggr.insert(tp_aggr.end(), tps.begin(), tps.end());
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
