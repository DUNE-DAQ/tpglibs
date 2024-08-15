/**
 * @file AbstractFactory.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGENGINE_ABSTRACTFACTORY_HPP_
#define TPGENGINE_ABSTRACTFACTORY_HPP_

#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

namespace tpgengine {

template <typename T>
class AbstractFactory {
  using create_processor_func = std::function<std::shared_ptr<T>()>;
  using name_creator_map = std::unordered_map<std::string, create_processor_func>;

  public:
    AbstractFactory() {}
    AbstractFactory(const AbstractFactory&) = delete;
    AbstractFactory& operator=(const AbstractFactory&) = delete;
    virtual ~AbstractFactory() = default;

    std::shared_ptr<T> create_processor(const std::string& processor_name);
    static void register_creator(const std::string& processor_name, create_processor_func creator);
    static std::shared_ptr<AbstractFactory<T>> get_instance();

  protected:
    static std::shared_ptr<AbstractFactory<T>> s_single_factory;

  private:
    static name_creator_map& get_creators();
};

} // namespace tpgengine

#include "tpgengine/AbstractFactory.hxx"

#endif // TPGENGINE_ABSTRACTFACTORY_HPP_
