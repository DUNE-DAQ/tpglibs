/**
 * @file AVXUtils.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include <immintrin.h>

#ifndef TPGLIBS_AVXUTILS_HPP_
#define TPGLIBS_AVXUTILS_HPP_

namespace tpglibs {

// NaiveUtils.hpp has the lay version.
inline __m256i _mm256_div_epi16(const __m256i& va, const int16_t& b) {
  __m256i vb = _mm256_set1_epi16(32768 / b);
  return _mm256_mulhrs_epi16(va, vb);
}

} // namespace tpglibs

#endif // TPGLIBS_AVXUTILS_HPP_
