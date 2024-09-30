/**
 * @file NaiveFactory.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_NAIVEFACTORY_HPP_
#define TPGLIBS_NAIVEFACTORY_HPP_

#include "tpglibs/AbstractFactory.hpp"
#include "tpglibs/NaiveProcessor.hpp"

/** @brief Factory registration macro. */
#define REGISTER_NAIVEPROCESSOR_CREATOR(processor_name, processor_class)                                                                                     \
    static struct processor_class##Registrar {                                                                                                               \
          processor_class##Registrar() {                                                                                                                     \
                  tpglibs::NaiveFactory::register_creator(processor_name, []() -> std::shared_ptr<tpglibs::NaiveProcessor> {return std::make_shared<processor_class>();});      \
                }                                                                                                                                            \
        } processor_class##_registrar;

namespace tpglibs {

/** @brief Naive typed abstract factory. */
class NaiveFactory : public AbstractFactory<NaiveProcessor> {};

} // namespace tpglibs

#endif // TPGLIBS_NAIVEFACTORY_HPP_
