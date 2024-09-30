/**
 * @file AbstractFactory.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_ABSTRACTFACTORY_HPP_
#define TPGLIBS_ABSTRACTFACTORY_HPP_

#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

namespace tpglibs {

/**
 * @brief General singleton, abstract factory.
 */
template <typename T>
class AbstractFactory {
  /** @brief Function that creates shared_ptrs of type T. */
  using create_processor_func = std::function<std::shared_ptr<T>()>;
  /** @brief Map from processor name to processor creation function. */
  using name_creator_map = std::unordered_map<std::string, create_processor_func>;

  public:
    AbstractFactory() {}
    AbstractFactory(const AbstractFactory&) = delete;
    AbstractFactory& operator=(const AbstractFactory&) = delete;
    virtual ~AbstractFactory() = default;

    /** @brief Create the requested processor
     *
     *  @param processor_name Name of the processor being created.
     *  @return shared_ptr of the requested processor. nullptr if it doesn't exist/isn't registered.
     */
    std::shared_ptr<T> create_processor(const std::string& processor_name);

    /** @brief Register the processor creation function to a given name.
     *
     *  @param processor_name String name of the processor to be registered as.
     *  @param creator Processor creation function.
     */
    static void register_creator(const std::string& processor_name, create_processor_func creator);

    /** @brief Singleton get instance function. */
    static std::shared_ptr<AbstractFactory<T>> get_instance();

  protected:
    /** @brief Singleton instance. */
    static std::shared_ptr<AbstractFactory<T>> s_single_factory;

  private:
    /** @brief Returns singleton instance of creation map. */
    static name_creator_map& get_creators();
};

} // namespace tpglibs

#include "tpglibs/AbstractFactory.hxx"

#endif // TPGLIBS_ABSTRACTFACTORY_HPP_
