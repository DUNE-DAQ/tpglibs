/**
 * @file AVXAbsRunSumProcessor.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpgengine/AVXRunSumProcessor.hpp"

#ifndef TPGENGINE_AVXABSRUNSUMPROCESSOR_HPP_
#define TPGENGINE_AVXABSRUNSUMPROCESSOR_HPP_

namespace tpgengine {

class AVXAbsRunSumProcessor : public AVXRunSumProcessor {
  public:
    __m256i process(const __m256i& signal) override;
};

} // namespace tpgengine

#endif // TPGENGINE_AVXABSRUNSUMPROCESSOR_HPP_
