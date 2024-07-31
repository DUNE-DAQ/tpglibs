/**
 * @file NaiveAbsRunSumProcessor.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpgengine/NaiveRunSumProcessor.hpp"

#include <cmath>

#ifndef TPGENGINE_NAIVEABSRUNSUMPROCESSOR_HPP_
#define TPGENGINE_NAIVEABSRUNSUMPROCESSOR_HPP_

namespace tpgengine {

class NaiveAbsRunSumProcessor : public NaiveRunSumProcessor {
  public:
    naive_array_t process(const naive_array_t& signal) override;
};

} // namespace tpgengine

#endif // TPGENGINE_NAIVEABSRUNSUMPROCESSOR_HPP_
