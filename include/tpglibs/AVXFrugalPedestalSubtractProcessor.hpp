/**
 * @file AVXFrugalPedestalSubtractProcessor.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AVXFactory.hpp"
#include "tpglibs/ProcessorMetricArray.hpp"
#include <atomic>
#include <memory>

#ifndef TPGLIBS_AVXFRUGALPEDESTALSUBTRACTPROCESSOR_HPP_
#define TPGLIBS_AVXFRUGALPEDESTALSUBTRACTPROCESSOR_HPP_

namespace tpglibs {

/** @brief AVX signal processor: Estimates the pedestal and subtracts.
 *
 *  Given a history of signals, this estimates the pedestal by shifting the current estimate when it is wrong in
 *  the same direction m_accum_limit times. For example, if the input signal is greater (less) than the estimated
 *  pedestal 10 (configurable) times in a row, then increment (decrement) the pedestal.
 */
class AVXFrugalPedestalSubtractProcessor : public AVXProcessor {
  protected:
    /** @brief Vector of estimated pedestals for each channel. */
    __m256i m_pedestal = _mm256_set1_epi16(0x4000);  // Set initial start to 14-bit max. Prevents garbage TPs at start.

    /** @brief Vector of counts that a channel's signal was above or below m_pedestal. */
    __m256i m_accum = _mm256_setzero_si256();

    /** @brief Count limit before committing to a pedestal shift. */
    int16_t m_accum_limit{10};

    /** @ Adjustable rate of storing metric to buffer, in terms of number of time process happens (time sample rate) */
    int64_t m_rate{512};
    uint64_t m_samples{0};
    bool m_collect_metric_flag{true};

  private:
    ProcessorMetricArray<__m256i> m_metric_store_buffers[2]{};
    // Initialize always to buffer 0 to make it safe, always points to one of buffer 0 and 1
    std::atomic<ProcessorMetricArray<__m256i>*> m_active_buffer = &m_metric_store_buffers[0];

    std::atomic<uint16_t>seq{0};

  public:
    /** @brief Allocate and initialize dual buffers */
    AVXFrugalPedestalSubtractProcessor();
    /** @brief Release buffer memory */
    ~AVXFrugalPedestalSubtractProcessor() noexcept;

    /** @brief Estimate the pedestal using the given signal and subtract.
     *
     *  @param signal A vector of channel signals.
     *  @return The input signal minus the estimated pedestal.
     */
    __m256i process(const __m256i& signal) override;

    /** @brief Configure the accumulation limit according to plane number.
     *
     *  @param config JSON config for the accumulation limits per plane.
     *  @param plane_numbers Array of plane numbers. Gives the channels to apply the accumulation limit.
     */
    void configure(const nlohmann::json& config, const int16_t* plane_numbers) override;

    /** @brief Save metrics to store buffer. */
    void save_metric_to_store_buffer() override;

    /** @brief returns the metrics being recorded and can be read by this processor
     * 
     * @return a vector of two strings: m_accum and m_pedestal
     */
    virtual std::vector<std::string> get_metric_items() override;

    /** @brief Read metrics from store buffer. */
    ProcessorMetricArray<__m256i> read_from_metric_store_buffer() override;

    std::string get_name() override;
};

} // namespace tpglibs

#endif // TPGLIBS_AVXFRUGALPEDESTALSUBTRACTPROCESSOR_HPP_
