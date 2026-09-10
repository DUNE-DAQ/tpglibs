/**
 * @file test_benchmark_tpg.cxx
 * @brief Self-contained benchmark and validation tool for TPG AVX2 SIMD optimizations.
 *
 * This tool compares the optimized AVX2 SIMD implementations with their original baselines
 * for bitwise equivalence and execution speed (using TSC CPU cycles and wall clock time).
 * It runs without any external DUNE-DAQ environment dependencies.
 */

#include <immintrin.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <iomanip>

// Mock TriggerPrimitive structure to avoid DUNE-DAQ dependencies
struct MockTriggerPrimitive {
    uint32_t adc_integral;
    uint16_t adc_peak;
    uint32_t channel;
    uint16_t samples_to_peak;
    uint16_t samples_over_threshold;
};

// Pipeline state struct for baseline and optimized pipeline comparisons
struct PipelineState {
    __m256i m_samples_over_threshold;
    __m256i m_adc_integral_lo;
    __m256i m_adc_integral_hi;
    __m256i m_adc_peak;
    __m256i m_samples_to_peak;

    void reset() {
        m_samples_over_threshold = _mm256_setzero_si256();
        m_adc_integral_lo = _mm256_setzero_si256();
        m_adc_integral_hi = _mm256_setzero_si256();
        m_adc_peak = _mm256_setzero_si256();
        m_samples_to_peak = _mm256_setzero_si256();
    }
};

// Helper to read invariant Time Stamp Counter (TSC)
inline uint64_t get_tsc_cycles() {
    unsigned int aux;
    return __rdtscp(&aux);
}

// ============================================================================
// BASELINE (ORIGINAL) IMPLEMENTATIONS
// ============================================================================

namespace baseline {

__m256i threshold_process(const __m256i& signal, const __m256i& threshold) {
    __m256i mask = _mm256_cmpgt_epi16(signal, threshold);
    __m256i above_threshold = _mm256_blendv_epi8(_mm256_setzero_si256(), signal, mask);
    return above_threshold;
}

__m256i frugal_pedestal_subtract(const __m256i& signal, __m256i& m_pedestal, __m256i& m_accum, const __m256i& m_accum_limit_reg) {
    __m256i is_gt = _mm256_cmpgt_epi16(signal, m_pedestal);
    __m256i is_lt = _mm256_cmpgt_epi16(m_pedestal, signal);

    __m256i to_add = _mm256_setzero_si256();
    to_add = _mm256_blendv_epi8(to_add, _mm256_set1_epi16(1), is_gt);
    to_add = _mm256_blendv_epi8(to_add, _mm256_set1_epi16(-1), is_lt);

    m_accum = _mm256_add_epi16(m_accum, to_add);

    is_gt = _mm256_cmpgt_epi16(m_accum, m_accum_limit_reg);
    is_lt = _mm256_cmpgt_epi16(_mm256_sub_epi16(_mm256_setzero_si256(), m_accum_limit_reg), m_accum);

    to_add = _mm256_setzero_si256();
    to_add = _mm256_blendv_epi8(to_add, _mm256_set1_epi16(1), is_gt);
    to_add = _mm256_blendv_epi8(to_add, _mm256_set1_epi16(-1), is_lt);

    m_pedestal = _mm256_adds_epi16(m_pedestal, to_add);

    __m256i need_reset = _mm256_or_si256(is_lt, is_gt);
    m_accum = _mm256_blendv_epi8(m_accum, _mm256_setzero_si256(), need_reset);

    return _mm256_sub_epi16(signal, m_pedestal);
}

__m256i pipeline_save_state(PipelineState& state, const __m256i& processed_signal, 
                            const __m256i& m_max_value_register, const __m256i& m_ones_register) {
    __m256i active       = _mm256_cmpgt_epi16(processed_signal, _mm256_setzero_si256());
    __m256i inactive     = _mm256_cmpeq_epi16(processed_signal, _mm256_setzero_si256());
    __m256i was_inactive = _mm256_cmpeq_epi16(state.m_samples_over_threshold, _mm256_setzero_si256());

    __m256i new_tps = _mm256_andnot_si256(was_inactive, inactive);

    __m256i adc_integral_sat = _mm256_adds_epu16(state.m_adc_integral_lo, processed_signal);
    state.m_adc_integral_lo = _mm256_add_epi16(state.m_adc_integral_lo, processed_signal);

    __m256i is_saturated = _mm256_cmpeq_epi16(adc_integral_sat, m_max_value_register);
    __m256i exact = _mm256_cmpeq_epi16(state.m_adc_integral_lo, adc_integral_sat);
    is_saturated  = _mm256_andnot_si256(exact, is_saturated);

    __m256i to_add = _mm256_and_si256(m_ones_register, is_saturated);
    state.m_adc_integral_hi = _mm256_adds_epu16(state.m_adc_integral_hi, to_add);

    __m256i above_peak = _mm256_cmpgt_epi16(processed_signal, state.m_adc_peak);

    state.m_adc_peak = _mm256_max_epi16(state.m_adc_peak, processed_signal);
    state.m_samples_to_peak = _mm256_blendv_epi8(state.m_samples_to_peak, state.m_samples_over_threshold, above_peak);

    __m256i time_add = _mm256_blendv_epi8(_mm256_setzero_si256(), m_ones_register, active);
    state.m_samples_over_threshold = _mm256_adds_epi16(state.m_samples_over_threshold, time_add);

    return new_tps;
}

void pipeline_generate_tps(PipelineState& state, const __m256i& tp_mask, 
                           uint16_t tp_sot[16], uint16_t tp_integral_lo[16], uint16_t tp_integral_hi[16], 
                           uint16_t tp_adc_peak[16], uint16_t tp_samples_to_peak[16]) {
    __m256i samples_over_threshold = _mm256_blendv_epi8(_mm256_setzero_si256(), state.m_samples_over_threshold, tp_mask);
    __m256i adc_integral_lo = _mm256_blendv_epi8(_mm256_setzero_si256(), state.m_adc_integral_lo, tp_mask);
    __m256i adc_integral_hi = _mm256_blendv_epi8(_mm256_setzero_si256(), state.m_adc_integral_hi, tp_mask);
    __m256i adc_peak = _mm256_blendv_epi8(_mm256_setzero_si256(), state.m_adc_peak, tp_mask);
    __m256i samples_to_peak = _mm256_blendv_epi8(_mm256_setzero_si256(), state.m_samples_to_peak, tp_mask);

    _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_sot), samples_over_threshold);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_integral_lo), adc_integral_lo);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_integral_hi), adc_integral_hi);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_adc_peak), adc_peak);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_samples_to_peak), samples_to_peak);

    state.m_samples_over_threshold = _mm256_blendv_epi8(state.m_samples_over_threshold, _mm256_setzero_si256(), tp_mask);
    state.m_adc_integral_lo     = _mm256_blendv_epi8(state.m_adc_integral_lo, _mm256_setzero_si256(), tp_mask);
    state.m_adc_integral_hi     = _mm256_blendv_epi8(state.m_adc_integral_hi, _mm256_setzero_si256(), tp_mask);
    state.m_adc_peak            = _mm256_blendv_epi8(state.m_adc_peak, _mm256_setzero_si256(), tp_mask);
    state.m_samples_to_peak     = _mm256_blendv_epi8(state.m_samples_to_peak, _mm256_setzero_si256(), tp_mask);
}

} // namespace baseline

// ============================================================================
// OPTIMIZED IMPLEMENTATIONS
// ============================================================================

namespace optimized {

__m256i threshold_process(const __m256i& signal, const __m256i& threshold) {
    __m256i mask = _mm256_cmpgt_epi16(signal, threshold);
    __m256i above_threshold = _mm256_and_si256(signal, mask);
    return above_threshold;
}

__m256i frugal_pedestal_subtract(const __m256i& signal, __m256i& m_pedestal, __m256i& m_accum, const __m256i& m_accum_limit_reg) {
    __m256i is_gt = _mm256_cmpgt_epi16(signal, m_pedestal);
    __m256i is_lt = _mm256_cmpgt_epi16(m_pedestal, signal);

    __m256i to_add = _mm256_sub_epi16(is_lt, is_gt);

    m_accum = _mm256_add_epi16(m_accum, to_add);

    is_gt = _mm256_cmpgt_epi16(m_accum, m_accum_limit_reg);
    is_lt = _mm256_cmpgt_epi16(_mm256_sub_epi16(_mm256_setzero_si256(), m_accum_limit_reg), m_accum);

    to_add = _mm256_sub_epi16(is_lt, is_gt);

    m_pedestal = _mm256_adds_epi16(m_pedestal, to_add);

    __m256i need_reset = _mm256_or_si256(is_lt, is_gt);
    m_accum = _mm256_andnot_si256(need_reset, m_accum);

    return _mm256_sub_epi16(signal, m_pedestal);
}

__m256i pipeline_save_state(PipelineState& state, const __m256i& processed_signal, 
                            const __m256i& m_max_value_register, const __m256i& m_ones_register) {
    __m256i active       = _mm256_cmpgt_epi16(processed_signal, _mm256_setzero_si256());
    __m256i inactive     = _mm256_cmpeq_epi16(processed_signal, _mm256_setzero_si256());
    __m256i was_inactive = _mm256_cmpeq_epi16(state.m_samples_over_threshold, _mm256_setzero_si256());

    __m256i new_tps = _mm256_andnot_si256(was_inactive, inactive);

    __m256i adc_integral_sat = _mm256_adds_epu16(state.m_adc_integral_lo, processed_signal);
    state.m_adc_integral_lo = _mm256_add_epi16(state.m_adc_integral_lo, processed_signal);

    __m256i is_saturated = _mm256_cmpeq_epi16(adc_integral_sat, m_max_value_register);
    __m256i exact = _mm256_cmpeq_epi16(state.m_adc_integral_lo, adc_integral_sat);
    is_saturated  = _mm256_andnot_si256(exact, is_saturated);

    __m256i to_add = _mm256_and_si256(m_ones_register, is_saturated);
    state.m_adc_integral_hi = _mm256_adds_epu16(state.m_adc_integral_hi, to_add);

    __m256i above_peak = _mm256_cmpgt_epi16(processed_signal, state.m_adc_peak);

    state.m_adc_peak = _mm256_max_epi16(state.m_adc_peak, processed_signal);
    state.m_samples_to_peak = _mm256_blendv_epi8(state.m_samples_to_peak, state.m_samples_over_threshold, above_peak);

    __m256i time_add = _mm256_and_si256(m_ones_register, active);
    state.m_samples_over_threshold = _mm256_adds_epi16(state.m_samples_over_threshold, time_add);

    return new_tps;
}

void pipeline_generate_tps(PipelineState& state, const __m256i& tp_mask, 
                           uint16_t tp_sot[16], uint16_t tp_integral_lo[16], uint16_t tp_integral_hi[16], 
                           uint16_t tp_adc_peak[16], uint16_t tp_samples_to_peak[16]) {
    __m256i samples_over_threshold = _mm256_and_si256(state.m_samples_over_threshold, tp_mask);
    __m256i adc_integral_lo = _mm256_and_si256(state.m_adc_integral_lo, tp_mask);
    __m256i adc_integral_hi = _mm256_and_si256(state.m_adc_integral_hi, tp_mask);
    __m256i adc_peak = _mm256_and_si256(state.m_adc_peak, tp_mask);
    __m256i samples_to_peak = _mm256_and_si256(state.m_samples_to_peak, tp_mask);

    _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_sot), samples_over_threshold);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_integral_lo), adc_integral_lo);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_integral_hi), adc_integral_hi);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_adc_peak), adc_peak);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(tp_samples_to_peak), samples_to_peak);

    state.m_samples_over_threshold = _mm256_andnot_si256(tp_mask, state.m_samples_over_threshold);
    state.m_adc_integral_lo     = _mm256_andnot_si256(tp_mask, state.m_adc_integral_lo);
    state.m_adc_integral_hi     = _mm256_andnot_si256(tp_mask, state.m_adc_integral_hi);
    state.m_adc_peak            = _mm256_andnot_si256(tp_mask, state.m_adc_peak);
    state.m_samples_to_peak     = _mm256_andnot_si256(tp_mask, state.m_samples_to_peak);
}

} // namespace optimized

// ============================================================================
// PATTERN GENERATORS
// ============================================================================

std::vector<__m256i> generate_sine_pattern(size_t size) {
    std::vector<__m256i> data(size);
    for (size_t i = 0; i < size; ++i) {
        int16_t temp[16];
        for (int j = 0; j < 16; ++j) {
            temp[j] = static_cast<int16_t>(500.0 * sin(2.0 * M_PI * (i + j) / 40.0) + 16000.0);
        }
        data[i] = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(temp));
    }
    return data;
}

std::vector<__m256i> generate_step_pattern(size_t size) {
    std::vector<__m256i> data(size);
    for (size_t i = 0; i < size; ++i) {
        int16_t temp[16];
        for (int j = 0; j < 16; ++j) {
            // Create sharp jumps to trigger threshold crossing and accumulator reset logic
            if ((i / 15) % 2 == 0) {
                temp[j] = 16500;
            } else {
                temp[j] = 15500;
            }
        }
        data[i] = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(temp));
    }
    return data;
}

std::vector<__m256i> generate_random_pattern(size_t size) {
    std::vector<__m256i> data(size);
    uint32_t state = 424242; // Fixed seed LCG
    auto next_rand = [&]() {
        state = state * 1103515245 + 12345;
        return static_cast<int16_t>(state & 0xFFFF);
    };

    for (size_t i = 0; i < size; ++i) {
        int16_t temp[16];
        for (int j = 0; j < 16; ++j) {
            temp[j] = 15000 + (next_rand() % 2000);
        }
        data[i] = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(temp));
    }
    return data;
}

// Helper to assert registers are identical
bool verify_registers_equal(const __m256i& a, const __m256i& b) {
    alignas(32) int16_t arr_a[16];
    alignas(32) int16_t arr_b[16];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(arr_a), a);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(arr_b), b);
    for (int i = 0; i < 16; ++i) {
        if (arr_a[i] != arr_b[i]) return false;
    }
    return true;
}

// ============================================================================
// HARNESS RUNNER
// ============================================================================

void run_benchmarks(const std::vector<__m256i>& inputs, const std::string& pattern_name) {
    std::cout << "------------------------------------------------------------\n";
    std::cout << "PATTERN: " << pattern_name << " (" << inputs.size() << " samples)\n";
    std::cout << "------------------------------------------------------------\n";

    const size_t NUM_RUNS = 50000;
    const size_t NUM_SAMPLES = inputs.size();

    // 1. Threshold Processor Validation & Benchmark
    {
        const __m256i threshold = _mm256_set1_epi16(16000);

        // Verification
        for (size_t i = 0; i < NUM_SAMPLES; ++i) {
            __m256i out_b = baseline::threshold_process(inputs[i], threshold);
            __m256i out_opt = optimized::threshold_process(inputs[i], threshold);
            if (!verify_registers_equal(out_b, out_opt)) {
                std::cerr << "ERROR: Threshold mismatch at index " << i << "\n";
                exit(1);
            }
        }

        // Benchmark
        {
            __m256i dummy_accum_b = _mm256_setzero_si256();
            auto start_wall = std::chrono::high_resolution_clock::now();
            uint64_t start_cycles = get_tsc_cycles();
            for (size_t r = 0; r < NUM_RUNS; ++r) {
                for (size_t i = 0; i < NUM_SAMPLES; ++i) {
                    __m256i out = baseline::threshold_process(inputs[i], threshold);
                    dummy_accum_b = _mm256_or_si256(dummy_accum_b, out);
                }
            }
            uint64_t end_cycles = get_tsc_cycles();
            auto end_wall = std::chrono::high_resolution_clock::now();
            
            alignas(32) int16_t temp[16];
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(temp), dummy_accum_b);
            volatile int16_t prevent_opt_b = temp[0];
            (void)prevent_opt_b;

            double base_time = std::chrono::duration<double, std::nano>(end_wall - start_wall).count() / (NUM_RUNS * NUM_SAMPLES);
            double base_cycles = static_cast<double>(end_cycles - start_cycles) / (NUM_RUNS * NUM_SAMPLES);

            // Benchmark Optimized
            __m256i dummy_accum_opt = _mm256_setzero_si256();
            start_wall = std::chrono::high_resolution_clock::now();
            start_cycles = get_tsc_cycles();
            for (size_t r = 0; r < NUM_RUNS; ++r) {
                for (size_t i = 0; i < NUM_SAMPLES; ++i) {
                    __m256i out = optimized::threshold_process(inputs[i], threshold);
                    dummy_accum_opt = _mm256_or_si256(dummy_accum_opt, out);
                }
            }
            end_cycles = get_tsc_cycles();
            end_wall = std::chrono::high_resolution_clock::now();

            _mm256_storeu_si256(reinterpret_cast<__m256i*>(temp), dummy_accum_opt);
            volatile int16_t prevent_opt_opt = temp[0];
            (void)prevent_opt_opt;

            double opt_time = std::chrono::duration<double, std::nano>(end_wall - start_wall).count() / (NUM_RUNS * NUM_SAMPLES);
            double opt_cycles = static_cast<double>(end_cycles - start_cycles) / (NUM_RUNS * NUM_SAMPLES);

            std::cout << std::left << std::setw(28) << "Threshold Processor:"
                      << "Baseline: " << std::fixed << std::setprecision(2) << base_cycles << " TSC-cyc (" << base_time << " ns) | "
                      << "Optimized: " << opt_cycles << " TSC-cyc (" << opt_time << " ns) | "
                      << "Speedup: " << ((base_time - opt_time) / base_time * 100.0) << "%\n";
        }
    }

    // 2. Frugal Pedestal Subtract Processor Validation & Benchmark
    {
        const __m256i limit = _mm256_set1_epi16(10);

        // Verification
        __m256i ped_b = _mm256_set1_epi16(16000);
        __m256i acc_b = _mm256_setzero_si256();
        __m256i ped_opt = _mm256_set1_epi16(16000);
        __m256i acc_opt = _mm256_setzero_si256();

        for (size_t i = 0; i < NUM_SAMPLES; ++i) {
            __m256i out_b = baseline::frugal_pedestal_subtract(inputs[i], ped_b, acc_b, limit);
            __m256i out_opt = optimized::frugal_pedestal_subtract(inputs[i], ped_opt, acc_opt, limit);
            if (!verify_registers_equal(out_b, out_opt) || 
                !verify_registers_equal(ped_b, ped_opt) || 
                !verify_registers_equal(acc_b, acc_opt)) {
                std::cerr << "ERROR: Frugal Pedestal Subtract state mismatch at index " << i << "\n";
                exit(1);
            }
        }

        // Benchmark
        {
            // Benchmark Baseline
            ped_b = _mm256_set1_epi16(16000);
            acc_b = _mm256_setzero_si256();
            __m256i dummy_accum_b = _mm256_setzero_si256();
            auto start_wall = std::chrono::high_resolution_clock::now();
            uint64_t start_cycles = get_tsc_cycles();
            for (size_t r = 0; r < NUM_RUNS; ++r) {
                for (size_t i = 0; i < NUM_SAMPLES; ++i) {
                    __m256i out = baseline::frugal_pedestal_subtract(inputs[i], ped_b, acc_b, limit);
                    dummy_accum_b = _mm256_or_si256(dummy_accum_b, out);
                }
            }
            uint64_t end_cycles = get_tsc_cycles();
            auto end_wall = std::chrono::high_resolution_clock::now();

            alignas(32) int16_t temp[16];
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(temp), dummy_accum_b);
            volatile int16_t prevent_opt_b = temp[0];
            (void)prevent_opt_b;

            double base_time = std::chrono::duration<double, std::nano>(end_wall - start_wall).count() / (NUM_RUNS * NUM_SAMPLES);
            double base_cycles = static_cast<double>(end_cycles - start_cycles) / (NUM_RUNS * NUM_SAMPLES);

            // Benchmark Optimized
            ped_opt = _mm256_set1_epi16(16000);
            acc_opt = _mm256_setzero_si256();
            __m256i dummy_accum_opt = _mm256_setzero_si256();
            start_wall = std::chrono::high_resolution_clock::now();
            start_cycles = get_tsc_cycles();
            for (size_t r = 0; r < NUM_RUNS; ++r) {
                for (size_t i = 0; i < NUM_SAMPLES; ++i) {
                    __m256i out = optimized::frugal_pedestal_subtract(inputs[i], ped_opt, acc_opt, limit);
                    dummy_accum_opt = _mm256_or_si256(dummy_accum_opt, out);
                }
            }
            end_cycles = get_tsc_cycles();
            end_wall = std::chrono::high_resolution_clock::now();

            _mm256_storeu_si256(reinterpret_cast<__m256i*>(temp), dummy_accum_opt);
            volatile int16_t prevent_opt_opt = temp[0];
            (void)prevent_opt_opt;

            double opt_time = std::chrono::duration<double, std::nano>(end_wall - start_wall).count() / (NUM_RUNS * NUM_SAMPLES);
            double opt_cycles = static_cast<double>(end_cycles - start_cycles) / (NUM_RUNS * NUM_SAMPLES);

            std::cout << std::left << std::setw(28) << "Frugal Pedestal Subtract:"
                      << "Baseline: " << std::fixed << std::setprecision(2) << base_cycles << " TSC-cyc (" << base_time << " ns) | "
                      << "Optimized: " << opt_cycles << " TSC-cyc (" << opt_time << " ns) | "
                      << "Speedup: " << ((base_time - opt_time) / base_time * 100.0) << "%\n";
        }
    }

    // 3. Pipeline Save State & Generate TPs Validation & Benchmark
    {
        const __m256i max_reg = _mm256_set1_epi16(0x7FFF);
        const __m256i ones_reg = _mm256_set1_epi16(1);

        // Verification
        PipelineState state_b; state_b.reset();
        PipelineState state_opt; state_opt.reset();

        for (size_t i = 0; i < NUM_SAMPLES; ++i) {
            __m256i processed_sig = _mm256_sub_epi16(inputs[i], _mm256_set1_epi16(16000));
            // Ensure positive signals only for saving state (active >= 0)
            processed_sig = _mm256_max_epi16(processed_sig, _mm256_setzero_si256());

            __m256i tp_mask_b = baseline::pipeline_save_state(state_b, processed_sig, max_reg, ones_reg);
            __m256i tp_mask_opt = optimized::pipeline_save_state(state_opt, processed_sig, max_reg, ones_reg);

            if (!verify_registers_equal(tp_mask_b, tp_mask_opt) ||
                !verify_registers_equal(state_b.m_samples_over_threshold, state_opt.m_samples_over_threshold) ||
                !verify_registers_equal(state_b.m_adc_integral_lo, state_opt.m_adc_integral_lo) ||
                !verify_registers_equal(state_b.m_adc_integral_hi, state_opt.m_adc_integral_hi) ||
                !verify_registers_equal(state_b.m_adc_peak, state_opt.m_adc_peak) ||
                !verify_registers_equal(state_b.m_samples_to_peak, state_opt.m_samples_to_peak)) {
                std::cerr << "ERROR: Pipeline save_state mismatch at index " << i << "\n";
                exit(1);
            }

            // Test Generate TPs if we have crossings
            uint16_t base_sot[16], base_int_lo[16], base_int_hi[16], base_peak[16], base_stop[16];
            uint16_t opt_sot[16], opt_int_lo[16], opt_int_hi[16], opt_peak[16], opt_stop[16];

            baseline::pipeline_generate_tps(state_b, tp_mask_b, base_sot, base_int_lo, base_int_hi, base_peak, base_stop);
            optimized::pipeline_generate_tps(state_opt, tp_mask_opt, opt_sot, opt_int_lo, opt_int_hi, opt_peak, opt_stop);

            if (std::memcmp(base_sot, opt_sot, 32) != 0 || std::memcmp(base_int_lo, opt_int_lo, 32) != 0 ||
                std::memcmp(base_int_hi, opt_int_hi, 32) != 0 || std::memcmp(base_peak, opt_peak, 32) != 0 ||
                std::memcmp(base_stop, opt_stop, 32) != 0 ||
                !verify_registers_equal(state_b.m_samples_over_threshold, state_opt.m_samples_over_threshold)) {
                std::cerr << "ERROR: Pipeline generate_tps mismatch at index " << i << "\n";
                exit(1);
            }
        }

        // Benchmark Save State
        {
            // Benchmark Baseline
            state_b.reset();
            __m256i dummy_accum_b = _mm256_setzero_si256();
            auto start_wall = std::chrono::high_resolution_clock::now();
            uint64_t start_cycles = get_tsc_cycles();
            for (size_t r = 0; r < NUM_RUNS; ++r) {
                for (size_t i = 0; i < NUM_SAMPLES; ++i) {
                    __m256i processed_sig = _mm256_sub_epi16(inputs[i], _mm256_set1_epi16(16000));
                    processed_sig = _mm256_max_epi16(processed_sig, _mm256_setzero_si256());
                    __m256i tp_mask = baseline::pipeline_save_state(state_b, processed_sig, max_reg, ones_reg);
                    dummy_accum_b = _mm256_or_si256(dummy_accum_b, tp_mask);
                }
            }
            uint64_t end_cycles = get_tsc_cycles();
            auto end_wall = std::chrono::high_resolution_clock::now();
            
            alignas(32) int16_t temp[16];
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(temp), dummy_accum_b);
            volatile int16_t prevent_opt_b = temp[0];
            (void)prevent_opt_b;

            double base_time_save = std::chrono::duration<double, std::nano>(end_wall - start_wall).count() / (NUM_RUNS * NUM_SAMPLES);
            double base_cycles_save = static_cast<double>(end_cycles - start_cycles) / (NUM_RUNS * NUM_SAMPLES);

            // Benchmark Optimized
            state_opt.reset();
            __m256i dummy_accum_opt = _mm256_setzero_si256();
            start_wall = std::chrono::high_resolution_clock::now();
            start_cycles = get_tsc_cycles();
            for (size_t r = 0; r < NUM_RUNS; ++r) {
                for (size_t i = 0; i < NUM_SAMPLES; ++i) {
                    __m256i processed_sig = _mm256_sub_epi16(inputs[i], _mm256_set1_epi16(16000));
                    processed_sig = _mm256_max_epi16(processed_sig, _mm256_setzero_si256());
                    __m256i tp_mask = optimized::pipeline_save_state(state_opt, processed_sig, max_reg, ones_reg);
                    dummy_accum_opt = _mm256_or_si256(dummy_accum_opt, tp_mask);
                }
            }
            end_cycles = get_tsc_cycles();
            end_wall = std::chrono::high_resolution_clock::now();

            _mm256_storeu_si256(reinterpret_cast<__m256i*>(temp), dummy_accum_opt);
            volatile int16_t prevent_opt_opt = temp[0];
            (void)prevent_opt_opt;

            double opt_time_save = std::chrono::duration<double, std::nano>(end_wall - start_wall).count() / (NUM_RUNS * NUM_SAMPLES);
            double opt_cycles_save = static_cast<double>(end_cycles - start_cycles) / (NUM_RUNS * NUM_SAMPLES);

            std::cout << std::left << std::setw(28) << "Pipeline Save State:"
                      << "Baseline: " << std::fixed << std::setprecision(2) << base_cycles_save << " TSC-cyc (" << base_time_save << " ns) | "
                      << "Optimized: " << opt_cycles_save << " TSC-cyc (" << opt_time_save << " ns) | "
                      << "Speedup: " << ((base_time_save - opt_time_save) / base_time_save * 100.0) << "%\n";
        }

        // Benchmark Generate TPs
        {
            // Benchmark Baseline
            uint16_t dummy_sot_b[16], dummy_lo_b[16], dummy_hi_b[16], dummy_peak_b[16], dummy_stop_b[16];
            const __m256i trigger_mask = _mm256_set_epi16(-1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0);
            state_b.reset();
            
            volatile uint16_t sum_sot_b = 0;
            volatile uint16_t sum_lo_b = 0;
            volatile uint16_t sum_hi_b = 0;
            volatile uint16_t sum_peak_b = 0;
            volatile uint16_t sum_stop_b = 0;

            auto start_wall = std::chrono::high_resolution_clock::now();
            uint64_t start_cycles = get_tsc_cycles();
            for (size_t r = 0; r < NUM_RUNS * 100; ++r) {
                // Re-populate state based on r so compiler cannot optimize loop away
                state_b.m_samples_over_threshold = _mm256_set1_epi16(r);
                state_b.m_adc_integral_lo = _mm256_set1_epi16(r);
                state_b.m_adc_integral_hi = _mm256_set1_epi16(r);
                state_b.m_adc_peak = _mm256_set1_epi16(r);
                state_b.m_samples_to_peak = _mm256_set1_epi16(r);

                baseline::pipeline_generate_tps(state_b, trigger_mask, dummy_sot_b, dummy_lo_b, dummy_hi_b, dummy_peak_b, dummy_stop_b);
                sum_sot_b = sum_sot_b + dummy_sot_b[0];
                sum_lo_b = sum_lo_b + dummy_lo_b[0];
                sum_hi_b = sum_hi_b + dummy_hi_b[0];
                sum_peak_b = sum_peak_b + dummy_peak_b[0];
                sum_stop_b = sum_stop_b + dummy_stop_b[0];
            }
            uint64_t end_cycles = get_tsc_cycles();
            auto end_wall = std::chrono::high_resolution_clock::now();

            double base_time_gen = std::chrono::duration<double, std::nano>(end_wall - start_wall).count() / (NUM_RUNS * 100);
            double base_cycles_gen = static_cast<double>(end_cycles - start_cycles) / (NUM_RUNS * 100);

            // Benchmark Optimized
            uint16_t dummy_sot_opt[16], dummy_lo_opt[16], dummy_hi_opt[16], dummy_peak_opt[16], dummy_stop_opt[16];
            state_opt.reset();

            volatile uint16_t sum_sot_opt = 0;
            volatile uint16_t sum_lo_opt = 0;
            volatile uint16_t sum_hi_opt = 0;
            volatile uint16_t sum_peak_opt = 0;
            volatile uint16_t sum_stop_opt = 0;

            start_wall = std::chrono::high_resolution_clock::now();
            start_cycles = get_tsc_cycles();
            for (size_t r = 0; r < NUM_RUNS * 100; ++r) {
                // Re-populate state based on r so compiler cannot optimize loop away
                state_opt.m_samples_over_threshold = _mm256_set1_epi16(r);
                state_opt.m_adc_integral_lo = _mm256_set1_epi16(r);
                state_opt.m_adc_integral_hi = _mm256_set1_epi16(r);
                state_opt.m_adc_peak = _mm256_set1_epi16(r);
                state_opt.m_samples_to_peak = _mm256_set1_epi16(r);

                optimized::pipeline_generate_tps(state_opt, trigger_mask, dummy_sot_opt, dummy_lo_opt, dummy_hi_opt, dummy_peak_opt, dummy_stop_opt);
                sum_sot_opt = sum_sot_opt + dummy_sot_opt[0];
                sum_lo_opt = sum_lo_opt + dummy_lo_opt[0];
                sum_hi_opt = sum_hi_opt + dummy_hi_opt[0];
                sum_peak_opt = sum_peak_opt + dummy_peak_opt[0];
                sum_stop_opt = sum_stop_opt + dummy_stop_opt[0];
            }
            end_cycles = get_tsc_cycles();
            end_wall = std::chrono::high_resolution_clock::now();

            double opt_time_gen = std::chrono::duration<double, std::nano>(end_wall - start_wall).count() / (NUM_RUNS * 100);
            double opt_cycles_gen = static_cast<double>(end_cycles - start_cycles) / (NUM_RUNS * 100);

            std::cout << std::left << std::setw(28) << "Pipeline Generate TPs:"
                      << "Baseline: " << std::fixed << std::setprecision(2) << base_cycles_gen << " TSC-cyc (" << base_time_gen << " ns) | "
                      << "Optimized: " << opt_cycles_gen << " TSC-cyc (" << opt_time_gen << " ns) | "
                      << "Speedup: " << ((base_time_gen - opt_time_gen) / base_time_gen * 100.0) << "%\n";
        }
    }
    std::cout << "\n";
}

int main() {
    std::cout << "========================================================================\n";
    std::cout << "      TPG AVX2 OPTIMIZATION VERIFICATION & BENCHMARK SUITE\n";
    std::cout << "========================================================================\n\n";

    std::cout << "NOTES ON BENCHMARKING METHODOLOGY:\n";
    std::cout << "1. cycles are measured using invariant TSC ('__rdtscp').\n";
    std::cout << "   On modern CPUs, TSC runs at a constant reference frequency.\n";
    std::cout << "2. Average wall time is measured using std::chrono::high_resolution_clock.\n";
    std::cout << "3. All runs verify bitwise-equivalence before measuring performance.\n\n";

    std::cout << "ALTERNATIVE profiling techniques:\n";
    std::cout << "- Linux hardware performance counters ('perf_event_open' / 'perf stat')\n";
    std::cout << "  measures actual cycles, instructions retired, cache and branch misses.\n";
    std::cout << "- Valgrind / Callgrind instruction simulation\n";
    std::cout << "  gives 100% deterministic instruction execution counts independent of CPU frequency.\n\n";

    // Generate patterns
    const size_t PATTERN_SIZE = 10000;
    auto sine_pattern = generate_sine_pattern(PATTERN_SIZE);
    auto step_pattern = generate_step_pattern(PATTERN_SIZE);
    auto random_pattern = generate_random_pattern(PATTERN_SIZE);

    run_benchmarks(sine_pattern, "Sine Wave Signal");
    run_benchmarks(step_pattern, "Step Signal (Threshold Jumps)");
    run_benchmarks(random_pattern, "Deterministic Random Noise");

    std::cout << "Verification Result: PASS (All outputs match bit-for-bit)\n";
    std::cout << "========================================================================\n";
    return 0;
}
