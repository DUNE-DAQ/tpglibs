/**
 * @file AVXProcessor.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpgengine/AbstractProcessor.hpp"

#include <immintrin.h>

#ifndef TPGENGINE_AVXPROCESSOR_HPP_
#define TPGENGINE_AVXPROCESSOR_HPP_

namespace tpgengine {

class AVXProcessor : public AbstractProcessor<__m256i> {
  public:
    virtual __m256i process(const __m256i& signal) override {
      return AbstractProcessor<__m256i>::process(signal);
    }
};

} // namespace tpgengine

#endif // TPGENGINE_AVXPROCESSOR_HPP_
