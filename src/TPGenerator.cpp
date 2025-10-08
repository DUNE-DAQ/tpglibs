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
                       const std::vector<std::pair<dunedaq::trgdataformats::channel_t, int16_t>> channel_plane_numbers,
                       const int sample_tick_difference) {
  m_num_pipelines = channel_plane_numbers.size() / m_num_channels_per_pipeline;
  m_sample_tick_difference = sample_tick_difference;

  for (const auto& name_config : configs) {
    if (name_config.second.contains("metric_collect_toggle_state") && name_config.second["metric_collect_toggle_state"] == true) {
      m_tpg_metric_collect_enabled = true;
    }
  }

  if (m_tpg_metric_collect_enabled) get_processor_metric_collector_ptr()->configure(configs, channel_plane_numbers, m_num_pipelines);

  for (int p = 0; p < m_num_pipelines; p++) {
    AVXPipeline new_pipe = AVXPipeline();
    auto begin_channel_plane = channel_plane_numbers.begin() + p*m_num_channels_per_pipeline;
    auto end_channel_plane = begin_channel_plane + m_num_channels_per_pipeline;
    new_pipe.configure(configs, std::vector<std::pair<dunedaq::trgdataformats::channel_t, int16_t>>(begin_channel_plane, end_channel_plane));
    new_pipe.set_sot_minima(m_sot_minima);
    m_tpg_pipelines.push_back(new_pipe);
  }

  int total_pipelines = m_tpg_pipelines.size();
  int start_index = (total_pipelines >= m_num_pipelines) ? (total_pipelines - m_num_pipelines) : 0;
  int pipeline_id = 0;
  for (int i = start_index; i < total_pipelines && pipeline_id < m_num_pipelines; ++i, ++pipeline_id) {
    auto& pipeline = m_tpg_pipelines[i];
    if (m_tpg_metric_collect_enabled) pipeline.attach_to_metric_collector(*get_processor_metric_collector_ptr(), pipeline_id);
    // I belive this is a current separate bug with repopulating the m_tpg_pipelines. Doing the safer treatment to take last pushed m_num_pipelines pipelines.
  }

  if (m_tpg_metric_collect_enabled) get_processor_metric_collector_ptr()->run();

}

std::shared_ptr<ProcessorMetricCollector<__m256i>>  TPGenerator::get_processor_metric_collector_ptr() {
  if (m_processor_metric_collector_ptr == nullptr) {
    m_processor_metric_collector_ptr = std::make_shared<ProcessorMetricCollector<__m256i>>();
  }
  return m_processor_metric_collector_ptr;
}

void TPGenerator::signal_metric_collection() {
  get_processor_metric_collector_ptr()->signal_collect();
}

std::vector<std::pair<std::shared_ptr<AbstractProcessor<__m256i>>, int>> TPGenerator::get_all_processor_references_with_pipeline_index() {
  std::vector<std::pair<std::shared_ptr<AbstractProcessor<__m256i>>, int>> processor_references;
  int total_pipelines = m_tpg_pipelines.size();
  int start_index = (total_pipelines >= m_num_pipelines) ? (total_pipelines - m_num_pipelines) : 0;
  int pipeline_id = 0;
  for (int i = start_index; i < total_pipelines && pipeline_id < m_num_pipelines; ++i, ++pipeline_id) {
    // @FIXME Restore to simple treatment, when repeated add of pipelines is fixed.
    for (auto& processor : m_tpg_pipelines[i].get_all_processor_references()) {
      processor_references.push_back(std::make_pair(processor, pipeline_id));
    }
  }
  return processor_references;
}

std::unordered_map<dunedaq::trgdataformats::channel_t, std::vector<std::pair<std::string, int16_t>>> TPGenerator::get_processor_metrics() {
  get_processor_metric_collector_ptr()->lock_metric_modify();

  auto metrics = get_processor_metric_collector_ptr()->get_metrics();

  get_processor_metric_collector_ptr()->unlock_metric_modify();

  return metrics;
}


void
TPGenerator::set_sot_minima(const std::vector<uint16_t>& sot_minima) {
  m_sot_minima = sot_minima;
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
