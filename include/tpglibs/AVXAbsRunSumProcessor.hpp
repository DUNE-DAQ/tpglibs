/**
 * @file AVXAbsRunSumProcessor.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AVXRunSumProcessor.hpp"

#ifndef TPGLIBS_AVXABSRUNSUMPROCESSOR_HPP_
#define TPGLIBS_AVXABSRUNSUMPROCESSOR_HPP_

namespace tpglibs {

class AVXAbsRunSumProcessor : public AVXRunSumProcessor {
  public:
    __m256i process(const __m256i& signal) override;
};

} // namespace tpglibs

#endif // TPGLIBS_AVXABSRUNSUMPROCESSOR_HPP_
