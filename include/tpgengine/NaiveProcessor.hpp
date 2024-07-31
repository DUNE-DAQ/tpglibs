/**
 * @file NaiveProcessor.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "AbstractProcessor.hpp"

#include <array>

#ifndef TPGENGINE_NAIVEPROCESSOR_HPP_
#define TPGENGINE_NAIVEPROCESSOR_HPP_

namespace tpgengine {

class NaiveProcessor : public AbstractProcessor<std::array<int16_t, 16>> {
  public:
    using naive_array_t = std::array<int16_t, 16>;

    virtual naive_array_t process(const naive_array_t& signal) override {
      return AbstractProcessor<naive_array_t>::process(signal);
    }
};

} // namespace tpgengine

#endif // TPGENGINE_NAIVEPROCESSOR_HPP_
