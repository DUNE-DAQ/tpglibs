/**
 * @file AVXAbsRunSumProcessor.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AVXRunSumProcessor.hpp"

#ifndef TPGLIBS_AVXABSRUNSUMPROCESSOR_HPP_
#define TPGLIBS_AVXABSRUNSUMPROCESSOR_HPP_

namespace tpglibs {

/** @brief AVX signal processor: Calculates the running sum of the signal's absolute value.
 *
 *  Look to AVXRunSumProcessor for details on the running sum calculation.
 */
class AVXAbsRunSumProcessor : public AVXRunSumProcessor {
  public:
    /** @brief Calculate and store the running sum with absolute values.
     *
     *  @param signal The input signal to process on.
     *  @return The calculated running sum.
     */
    __m256i process(const __m256i& signal) override;
};

} // namespace tpglibs

#endif // TPGLIBS_AVXABSRUNSUMPROCESSOR_HPP_
