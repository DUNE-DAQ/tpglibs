/**
 * @file NaiveAbsRunSumProcessor.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/NaiveRunSumProcessor.hpp"

#include <cmath>

#ifndef TPGLIBS_NAIVEABSRUNSUMPROCESSOR_HPP_
#define TPGLIBS_NAIVEABSRUNSUMPROCESSOR_HPP_

namespace tpglibs {

/** @brief Naive signal processor: Calculates the running sum of the signal's absolute value.
 *
 *  Look to NaiveRunSumProcessor for details on the running sum calculation.
 */
class NaiveAbsRunSumProcessor : public NaiveRunSumProcessor {
  public:
    /** @brief Calculate and store the running sum with absolute values.
     *
     *  @param signal The input signal to process on.
     *  @return The calculated running sum.
     */
    naive_array_t process(const naive_array_t& signal) override;
};

} // namespace tpglibs

#endif // TPGLIBS_NAIVEABSRUNSUMPROCESSOR_HPP_
