/**
 * @file NaiveFactory.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGENGINE_NAIVEFACTORY_HPP_
#define TPGENGINE_NAIVEFACTORY_HPP_

#include "tpgengine/AbstractFactory.hpp"
#include "tpgengine/NaiveProcessor.hpp"

#define REGISTER_NAIVEPROCESSOR_CREATOR(processor_name, processor_class)                                                                                     \
    static struct processor_class##Registrar {                                                                                                               \
          processor_class##Registrar() {                                                                                                                     \
                  tpgengine::NaiveFactory::register_creator(processor_name, []() -> std::shared_ptr<tpgengine::NaiveProcessor> {return std::make_shared<processor_class>();});      \
                }                                                                                                                                            \
        } processor_class##_registrar;

namespace tpgengine {

  class NaiveFactory : public AbstractFactory<NaiveProcessor> {};

} // namespace tpgengine

#endif // TPGENGINE_NAIVEFACTORY_HPP_
