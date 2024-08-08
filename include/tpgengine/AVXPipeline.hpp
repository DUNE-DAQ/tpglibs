/**
 * @file AVXPipeline.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGENGINE_AVXPIPELINE_HPP_
#define TPGENGINE_AVXPIPELINE_HPP_

#include "tpgengine/TPGPipeline.hpp"
#include "tpgengine/AVXFactory.hpp"

namespace tpgengine {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
class AVXPipeline : public TPGPipeline<AVXProcessor, __m256i> {
  public:
    __m256i save_state(const __m256i& processed_signal) override;
    bool check_for_tps(const __m256i& tp_mask) override;
    std::vector<dunedaq::trgdataformats::TriggerPrimitive> generate_tps(const __m256i& tp_mask) override;
};
#pragma GCC diagnostic pop

} // namespace tpgengine

#endif // TPGENGINE_AVXPIPELINE_HPP_
