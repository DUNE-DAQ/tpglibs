/**
 * @file NaiveUtils.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include <cstdint>

#ifndef TPGLIBS_NAIVEUTILS_HPP_
#define TPGLIBS_NAIVEUTILS_HPP_

namespace tpglibs {

// Modeled to be consistent with AVX.
/** @brief Naive model of AVX division in AVXUtils.
 *
 *  This is to produce consistent results with AVX division. The rounding is unorthodox,
 *  but it is systematic.
 *
 *  @param a The dividend.
 *  @param b The divisor.
 *  @return a / b with funky rounding.
 */
inline int16_t _naive_div_int16(const int16_t& a, const int16_t& b) {
  int16_t vb = (1 << 15) / b;         //  1 / b * 2^15
  int32_t mulhrs = a * vb;            //  a / b * 2^15
  mulhrs = (mulhrs >> 14) + 1;        // (a / b * 2^15) * 2^-14 + 1 ~ a / b * 2 + 1
  mulhrs = mulhrs >> 1;               //~ a / b. The +1 causes unorthodox rounding.
  return (int16_t)(mulhrs);
}

} // namespace tpglibs

#endif // TPGLIBS_NAIVEUTILS_HPP_
