/**
 * @file TPGenerator.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpgengine/TPGenerator.hpp"

namespace tpgengine {

void
TPGenerator::configure(const std::vector<nlohmann::json>& configs,
                       const std::vector<std::pair<int16_t, int16_t>> channel_plane_numbers) {
  m_num_pipelines = channel_plane_numbers.size() / m_num_channels_per_pipeline;

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


} // namespace tpgengine
