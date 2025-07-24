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
  if (config.contains("metric_collect_data_sample_rate")) m_rate = config["metric_collect_data_sample_rate"];
}

__m256i AVXFrugalPedestalSubtractProcessor::process(const __m256i& signal) {
  // save metric

  if (m_samples++ % m_rate == 0) save_metric_to_store_buffer();

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
  auto free_ptr = m_active_buffer.load(std::memory_order_acquire);
  // write to free buffer
  
  //set seq
  seq.fetch_add(1, std::memory_order_relaxed);
  
  free_ptr->m_data[0] = m_pedestal;
  free_ptr->m_data[1] = m_accum;

  //set seq to indicate write end
  seq.fetch_add(1, std::memory_order_release);
}

ProcessorMetricArray<__m256i> AVXFrugalPedestalSubtractProcessor::read_from_metric_store_buffer() {
  // "read" is expected to happen at a much lower frequency than store
  // Therefore, the responsibility to switch buffers given to it

  uint16_t start, end;
  ProcessorMetricArray<__m256i>* active_buffer_curr;
  do {
    start = seq.load(std::memory_order_acquire);
    if (start & 1) continue; // If odd, then writer is currently writing
    active_buffer_curr = m_active_buffer.load(std::memory_order_acquire);
    m_active_buffer.store(active_buffer_curr == &m_metric_store_buffers[0] ? &m_metric_store_buffers[1] : &m_metric_store_buffers[0], std::memory_order_release);
    end = seq.load(std::memory_order_acquire);
  } while (start != end);
  // after the switch, the processor should be writing into the backup buffer now, we can safely readout its value

  return *active_buffer_curr;
}

std::string AVXFrugalPedestalSubtractProcessor::get_name() {
  return "AVXFrugalPedestalSubtractProcessor";
}

} // namespace tpglibs
