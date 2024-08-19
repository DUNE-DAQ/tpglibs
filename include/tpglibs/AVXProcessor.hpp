/**
 * @file AVXProcessor.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/AbstractProcessor.hpp"

#include <immintrin.h>

#ifndef TPGENGINE_AVXPROCESSOR_HPP_
#define TPGENGINE_AVXPROCESSOR_HPP_

namespace tpglibs {

// Compilation warns about some AVX alignment attributes that are ignored.
// This use case should not worry about these warnings.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
class AVXProcessor : public AbstractProcessor<__m256i> {
  public:
    virtual __m256i process(const __m256i& signal) override {
      return AbstractProcessor<__m256i>::process(signal);
    }
};
#pragma GCC diagnostic pop

} // namespace tpglibs

#endif // TPGENGINE_AVXPROCESSOR_HPP_
