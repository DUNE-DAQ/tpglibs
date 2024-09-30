/**
 * @file TPGenerator.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/TPGenerator.hpp"

namespace tpglibs {

void
TPGenerator::configure(const std::vector<std::pair<std::string, nlohmann::json>>& configs,
                       const std::vector<std::pair<int16_t, int16_t>> channel_plane_numbers,
                       const int sample_tick_difference) {
  m_num_pipelines = channel_plane_numbers.size() / m_num_channels_per_pipeline;
  m_sample_tick_difference = sample_tick_difference;

  for (int p = 0; p < m_num_pipelines; p++) {
    AVXPipeline new_pipe = AVXPipeline();
    auto begin_channel_plane = channel_plane_numbers.begin() + p*m_num_channels_per_pipeline;
    auto end_channel_plane = begin_channel_plane + m_num_channels_per_pipeline;
    new_pipe.configure(configs, std::vector<std::pair<int16_t, int16_t>>(begin_channel_plane, end_channel_plane));
    m_tpg_pipelines.push_back(new_pipe);
  }
}

__m256i
TPGenerator::expand_frame(const __m256i& regi) {
  // Refer to the diagram and documentation on frame expansion for details.

  // Prepare even (2,4,6,8), odd (1,3,5,7) rows in 64-bit sense.
  __m256i odd  = _mm256_permutevar8x32_epi32(regi, _mm256_setr_epi32(1, 0, 1, 2, 3, 4, 5, 6));

  // Shift into place.
  __m256i even = _mm256_sllv_epi64(regi, _mm256_setr_epi64x(6, 14, 22, 30));
  odd  = _mm256_srlv_epi64(odd, _mm256_setr_epi64x(30, 22, 14, 6));

  // Everything is center aligned in 32-bit. Mask and right-align the right side.
  __m256i both  = _mm256_blend_epi32(even, odd, 0b01010101);
  __m256i right = _mm256_and_si256(_mm256_set1_epi32(0xFFFFu), both);
  __m256i left  = _mm256_and_si256(_mm256_set1_epi32(0x3FFF0000u), both);

  right = _mm256_srli_epi32(right, 2);
  return _mm256_or_si256(left, right);
}

__m256i
TPGenerator::old_expand_frame(const __m256i& regi) {
  // Refer to the diagram and documentation on frame expansion for details.

  // Rearrange original with row 3 doubled.
  __m256i idx = _mm256_set_epi32(6, 5, 4, 3, 3, 2, 1, 0);
  __m256i shuf1 = _mm256_permutevar8x32_epi32(regi, idx);

  // Left shift each row.
  __m256i count = _mm256_set_epi32(12, 8, 4, 0, 14, 10, 6, 2);
  __m256i high_half = _mm256_sllv_epi32(shuf1, count);
  high_half = _mm256_and_si256(high_half, _mm256_set1_epi32(0x3FFF0000u));  // Mask out the low half.

  // Left shift for low half later.
  count = _mm256_set_epi32(10, 6, 2, 0, 12, 8, 4, 0);
  __m256i shift2 = _mm256_sllv_epi32(shuf1, count);

  // Rearrange original and doubled rows 2 and 0.
  idx = _mm256_set_epi32(5, 4, 3, 2, 2, 1, 0, 0);
  __m256i shuf2 = _mm256_permutevar8x32_epi32(regi, idx);

  // Right shift each row.
  count = _mm256_set_epi32(22, 26, 30, 0, 20, 24, 28, 0);
  __m256i shift3 = _mm256_srlv_epi32(shuf2, count);

  // "Complete" the low half. Still more.
  __m256i low_half = _mm256_or_si256(shift2, shift3);
  low_half = _mm256_and_si256(low_half, _mm256_set1_epi32(0x3FFFu)); // Mask out the high half.

  // Combine halves and clear space for an odd entry.
  __m256i both = _mm256_or_si256(low_half, high_half);
  both = _mm256_andnot_si256(_mm256_set_epi32(0, 0, 0, 0xFFFFu, 0, 0, 0, 0), both);

  // There is a specific 16-bit entry that needs special handling.
  // Align it.
  __m256i shift4 = _mm256_srli_epi32(regi, 18);
  // Mask it.
  shift4 = _mm256_and_si256(_mm256_set_epi32(0, 0x3FFFu, 0, 0, 0, 0, 0, 0), shift4);

  // Permute into the right spot
  idx = _mm256_set_epi32(0, 0, 0, 6, 0, 0, 0, 0);
  __m256i shuf3 = _mm256_permutevar8x32_epi32(shift4, idx);

  // Add it in.
  both = _mm256_or_si256(both, shuf3);
  return both;
}


} // namespace tpglibs
