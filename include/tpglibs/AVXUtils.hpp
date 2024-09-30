/**
 * @file AVXUtils.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include <immintrin.h>
#include <fmt/core.h>
#include <fmt/ranges.h>

#include <array>

#ifndef TPGLIBS_AVXUTILS_HPP_
#define TPGLIBS_AVXUTILS_HPP_

namespace tpglibs {

/**
 * @brief Hack-ish AVX division function.
 *
 * Derivation from https://stackoverflow.com/questions/42442325/how-to-divide-a-m256i-vector-by-an-integer-variable
 *
 * @param va AVX2 register as the dividend.
 * @param b Integer as the divisor.
 * @return AVX2 register whose elements were divided by b.
 */
inline __m256i _mm256_div_epi16(const __m256i& va, const int16_t& b) {
  // NaiveUtils.hpp has the lay version.
  __m256i vb = _mm256_set1_epi16(32768 / b);
  return _mm256_mulhrs_epi16(va, vb);
}


inline void _mm256_print_epi16(const __m256i& input) {
  std::array<int16_t, 16> prints;
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(prints.begin()), input);

  fmt::print("{}", prints);
}

} // namespace tpglibs

#endif // TPGLIBS_AVXUTILS_HPP_
