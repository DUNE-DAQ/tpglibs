/**
 * @file NaiveAbsRunSumProcessor.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "tpglibs/NaiveAbsRunSumProcessor.hpp"

namespace tpglibs {

REGISTER_NAIVEPROCESSOR_CREATOR("NaiveAbsRunSumProcessor", NaiveAbsRunSumProcessor)

NaiveAbsRunSumProcessor::naive_array_t NaiveAbsRunSumProcessor::process(const naive_array_t& signal) {
  naive_array_t abs_signal;

  for (int i = 0; i < 16; i++) {
    abs_signal[i] = std::abs(signal[i]);
  }

  return NaiveRunSumProcessor::process(abs_signal);
}

} // namespace tpglibs
