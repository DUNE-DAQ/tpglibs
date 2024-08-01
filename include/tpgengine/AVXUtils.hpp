/**
 * @file AVXUtils.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include <immintrin.h>

#ifndef TPGENGINE_AVXUTILS_HPP_
#define TPGENGINE_AVXUTILS_HPP_

namespace tpgengine {

// NaiveUtils.hpp has the lay version.
inline __m256i _mm256_div_epi16(const __m256i& va, const int16_t& b) {
  __m256i vb = _mm256_set1_epi16(32768 / b);
  return _mm256_mulhrs_epi16(va, vb);
}

} // namespace tpgengine

#endif // TPGENGINE_AVXUTILS_HPP_
