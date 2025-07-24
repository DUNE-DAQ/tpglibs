/**
 * @file TPGenerator.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_TPGENERATOR_HPP_
#define TPGLIBS_TPGENERATOR_HPP_

#include "tpglibs/AVXPipeline.hpp"

#include "tpglibs/Types.hpp"

#include <utility>

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
  bool m_expand_frames;
  std::vector<std::atomic<int>>* m_tp_count_per_channel;

  public:
    /**
     * @brief Setup and configure the AVX pipelines.
     *
     * @param configs A vector of pairs: AVX pipeline to use and its configuration.
     * @param channel_plane_numbers A vector of channel numbers and their plane numbers.
     * @param sample_tick_difference Number of ticks between time samples in expected data frames.
     * @param expand_frames Whether expansion should be performed on each frame.
     */
    void configure(const std::vector<std::pair<std::string, nlohmann::json>>& configs,
                   const std::vector<std::pair<channel_t, int16_t>> channel_plane_numbers,
                   const int sample_tick_difference,
                   const bool expand_frames,
                   std::vector<std::atomic<int>>* tp_count_per_channel);

    /**
     * @brief Set the minimum samples over threshold for a TP according to plane.
     *
     * @param sot_minima TPs from plane `i` will have at least `sot_minima[i]` value for its samples_over_threshold.
     */
    void set_sot_minima(const std::vector<uint16_t>& sot_minima);

    /**
     * @brief Driving function for the TPG.
     *
     * This function receives the frames, expands, sends down AVX pipelines, and returns TPs that were generated.
     *
     * @param frame A data frame to process and generate TPs from.
     * @return A vector of TPs.
     */
    template <typename T>
    void operator()(const T* frame) {
      // Max number of TPs for a channel: number of time samples / 2.
      //std::vector<TriggerPrimitive> tp_aggr;
      //tp_aggr.reserve(T::s_num_channels * T::s_time_samples_per_frame / 2);

      const int bits_per_adc = m_expand_frames ? T::s_bits_per_adc : 16;

      const typename T::word_t* words_ptr = reinterpret_cast<const typename T::word_t*>(frame->adc_words);
      const uint64_t timestamp = frame->get_timestamp();

      const int register_alignment = bits_per_adc * m_num_channels_per_pipeline;
      // Loop in time.
      for (int t = 0; t < T::s_time_samples_per_frame; t++) {
        const typename T::word_t* time_sample = words_ptr + t * bits_per_adc;
        char* cursor = reinterpret_cast<char*>(const_cast<typename T::word_t*>(time_sample)); // Need to walk in terms of bytes/bits.

        // Loop in pipelines.
        for (int p = 0; p < m_num_pipelines; p++) {
          if (m_expand_frames && p == m_num_pipelines - 1)
            cursor -= 4; // Take a step of 32 bit backwards for the last sub-frame.

          __m256i regi = _mm256_lddqu_si256((__m256i*)cursor);

          if (m_expand_frames && p == m_num_pipelines - 1) // Permute the row order to use the same operation.
            regi = _mm256_permutevar8x32_epi32(regi, _mm256_setr_epi32(1, 2, 3, 4, 5, 6, 7, 0));

          __m256i expanded_subframe = m_expand_frames ? expand_frame(regi) : regi;
          /*std::vector<TriggerPrimitive> tps = */m_tpg_pipelines[p].process(expanded_subframe, m_tp_count_per_channel);

          //for (auto tp : tps) {
          //  tp.time_start = (t - tp.samples_over_threshold) * m_sample_tick_difference + timestamp;
          //  //tp_aggr.push_back(tp);
          //}
          cursor += register_alignment / 8; // Numerator is in bits. Need bytes.
        }
      }

      //return tp_aggr;
    }

  private:
    __m256i expand_frame(const __m256i& regi); /// @brief Expansion from 14-bit signals to 16-bit.
    __m256i old_expand_frame(const __m256i& regi); /// @brief Legacy expansion function. 
};

} // namespace tpglibs

#endif // TPGLIBS_TPGENERATOR_HPP_
