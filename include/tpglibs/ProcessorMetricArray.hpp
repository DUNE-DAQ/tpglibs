/**
 * @file ProcessorMetricArray.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_PROCESSORMETRICARRAY_HPP_
#define TPGLIBS_PROCESSORMETRICARRAY_HPP_

#include <cstddef>
#include <immintrin.h>

namespace tpglibs {

/// @brief Dynamic array of processor metrics, templated on signal type.
template <typename signal_type_t>
struct ProcessorMetricArray {
  signal_type_t* m_data;      ///< Pointer to contiguous metric data
  std::size_t m_size;         ///< Number of metrics in the array
};

} // namespace tpglibs

#endif // TPGLIBS_PROCESSORMETRICARRAY_HPP_