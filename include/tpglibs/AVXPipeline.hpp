/**
 * @file AVXPipeline.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_AVXPIPELINE_HPP_
#define TPGLIBS_AVXPIPELINE_HPP_

#include "tpglibs/TPGPipeline.hpp"
#include "tpglibs/AVXFactory.hpp"

namespace tpglibs {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
/** @brief AVX typed TPG pipeline. */
class AVXPipeline : public TPGPipeline<AVXProcessor, __m256i> {
  public:
    /** @brief Save the state of the processed signals.
     *
     *  Used for keeping track of TPs that are still on-going.
     *
     *  @param processed_signal Resultant signal after completing the pipeline.
     *  @return A vector mask of channels that have completed TPs.
     */
    __m256i save_state(const __m256i& processed_signal) override;

    /** @brief Check a channel mask for any TPs that need to be created.
     *
     *  @param tp_mask A vector mask of channels that have completed TPs.
     *  @return True if at least 1 channel has a completed TP. False otherwise.
     */
    bool check_for_tps(const __m256i& tp_mask) override;

    /** @brief Finalize the details of the completed TPs and send out.
     *
     *  @param tp_mask A vector mask of channels that have completed TPs.
     *  @return A vector of completed TPs.
     */
    std::vector<dunedaq::trgdataformats::TriggerPrimitive> generate_tps(const __m256i& tp_mask) override;
};
#pragma GCC diagnostic pop

} // namespace tpglibs

#endif // TPGLIBS_AVXPIPELINE_HPP_
