/**
 * @file AVXFrugalPedestalSubtractProcessor.cpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AVXFrugalPedestalSubtractProcessor.hpp"
#include "tpglibs/ProcessorMetricArray.hpp"
#include <mm_malloc.h>

namespace tpglibs {

REGISTER_AVXPROCESSOR_CREATOR("AVXFrugalPedestalSubtractProcessor", AVXFrugalPedestalSubtractProcessor)

AVXFrugalPedestalSubtractProcessor::AVXFrugalPedestalSubtractProcessor() {
  // Initialize the static arrays in metric store buffer
  for (auto& buf : m_metric_store_buffers) {
    buf.m_size = 2; // Storing m_pedetal and m_acuum for this type of processor
    buf.m_data = static_cast<__m256i*>(
      _mm_malloc(buf.m_size * sizeof(__m256i), alignof(__m256i))
    );
  }
}

AVXFrugalPedestalSubtractProcessor::~AVXFrugalPedestalSubtractProcessor() noexcept {
  for (auto buf : m_metric_store_buffers) {
    _mm_free(buf.m_data);
  }
}

void AVXFrugalPedestalSubtractProcessor::configure(const nlohmann::json& config, const int16_t* plane_numbers) {
  m_accum_limit = config["accum_limit"];
  if (config.contains("metric_collect_time_sample_period")) m_sample_period = config["metric_collect_time_sample_period"];
  if (config.contains("metric_collect_toggle_state")) m_collect_metric_flag = config["metric_collect_toggle_state"];

  initialize_internal_state_collection();

  m_internal_state_name_registry->register_internal_state("pedestal", std::make_shared<__m256i>(m_pedestal));
  m_internal_state_name_registry->register_internal_state("accum", std::make_shared<__m256i>(m_accum));

  // @FIXME temporary override before changing configs
  m_internal_state_name_registry->parse_requested_internal_state_items("pedestal");
  m_internal_state_buffer_manager->configure_from_registry(m_internal_state_name_registry);
}

__m256i AVXFrugalPedestalSubtractProcessor::process(const __m256i& signal) {
  // save metric

  if (m_collect_metric_flag && m_samples++ % m_sample_period == 0) save_metric_to_store_buffer();

  // @FIXME parallel call to the new structure
  if (m_collect_metric_flag && m_samples++ % m_sample_period == 0) m_internal_state_buffer_manager->write_to_active_buffer();

  // Find the channels that are above or below the pedestal.
  __m256i is_gt = _mm256_cmpgt_epi16(signal, m_pedestal);
  __m256i is_lt = _mm256_cmpgt_epi16(m_pedestal, signal);

  // Update m_accum.
  __m256i to_add = _mm256_setzero_si256();                                    // Assumes equal to pedestal.
  to_add = _mm256_blendv_epi8(to_add, _mm256_set1_epi16(1), is_gt);           // Set the above pedestal case.
  to_add = _mm256_blendv_epi8(to_add, _mm256_set1_epi16(-1), is_lt);          // Set the below pedestal case.

  m_accum = _mm256_add_epi16(m_accum, to_add);

  // Check the accum limit condition.
  is_gt = _mm256_cmpgt_epi16(m_accum, _mm256_set1_epi16(m_accum_limit));      // m_accum > +limit.
  is_lt = _mm256_cmpgt_epi16(_mm256_set1_epi16(-1*m_accum_limit), m_accum);   // m_accum < -limit = -limit > m_accum.

  to_add = _mm256_setzero_si256();
  to_add = _mm256_blendv_epi8(to_add, _mm256_set1_epi16(1), is_gt);
  to_add = _mm256_blendv_epi8(to_add, _mm256_set1_epi16(-1), is_lt);

  // Update pedestal.
  m_pedestal = _mm256_adds_epi16(m_pedestal, to_add);

  // Reset too high/low m_accum channels.
  __m256i need_reset = _mm256_or_si256(is_lt, is_gt);
  m_accum = _mm256_blendv_epi8(m_accum, _mm256_setzero_si256(), need_reset);

  return AVXProcessor::process(_mm256_sub_epi16(signal, m_pedestal));
}

void AVXFrugalPedestalSubtractProcessor::save_metric_to_store_buffer() {
  // Store is expected to happen at a much higher frequency than read, for example,
  // can be store after each processing
  // Therefore, "save" should have no interrupt or wait

  //set seq
  seq.fetch_add(1, std::memory_order_release);
  auto free_ptr = m_active_buffer.load(std::memory_order_acquire);
  // write to free buffer
  
  free_ptr->m_data[0] = m_pedestal;
  free_ptr->m_data[1] = m_accum;

  //set seq to indicate write end
  seq.fetch_add(1, std::memory_order_release);
}

ProcessorMetricArray<__m256i> AVXFrugalPedestalSubtractProcessor::read_from_metric_store_buffer() {
  // Wait until no write is in progress.
  if (!m_collect_metric_flag) return {};
  uint16_t start_seq;
  do {
    start_seq = seq.load(std::memory_order_acquire);
  } while (start_seq & 1); // spin if writer is mid-write

  // Swap the active buffer so writer will go to the other one.
  auto current_active = m_active_buffer.load(std::memory_order_acquire);
  auto new_active = (current_active == &m_metric_store_buffers[0])
                      ? &m_metric_store_buffers[1]
                      : &m_metric_store_buffers[0];
  m_active_buffer.store(new_active, std::memory_order_release);

  // Now it's safe to read from the previous active buffer.
  return *current_active;
}

std::vector<std::string> AVXFrugalPedestalSubtractProcessor::get_metric_items() {
  if (!m_collect_metric_flag) return {};
  return {"m_pedestal", "m_accum"};
}

} // namespace tpglibs
