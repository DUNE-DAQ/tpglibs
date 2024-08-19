/**
 * @file AVXAbsRunSumProcessor.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AVXAbsRunSumProcessor.hpp"

namespace tpglibs {

REGISTER_AVXPROCESSOR_CREATOR("AVXAbsRunSumProcessor", AVXAbsRunSumProcessor)

__m256i AVXAbsRunSumProcessor::process(const __m256i& signal) {
  return AVXRunSumProcessor::process(_mm256_abs_epi16(signal));
}

} // namespace tpglibs
