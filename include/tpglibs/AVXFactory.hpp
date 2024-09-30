/**
 * @file AVXFactory.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_AVXFACTORY_HPP_
#define TPGLIBS_AVXFACTORY_HPP_

#include "tpglibs/AbstractFactory.hpp"
#include "tpglibs/AVXProcessor.hpp"

/** @brief Factory registration macro. */
#define REGISTER_AVXPROCESSOR_CREATOR(processor_name, processor_class)                                                                     \
  static struct processor_class##Registrar {                                                                                               \
    processor_class##Registrar() {                                                                                                         \
      tpglibs::AVXFactory::register_creator(processor_name, []() -> std::shared_ptr<tpglibs::AVXProcessor> {return std::make_shared<processor_class>();});      \
    }                                                                                                                                      \
  } processor_class##_registrar;

namespace tpglibs {

/** @brief AVX typed abstract factory. */
class AVXFactory : public AbstractFactory<AVXProcessor> {};

} // namespace tpglibs

#endif // TPGLIBS_AVXFACTORY_HPP_
