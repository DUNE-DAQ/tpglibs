/**
 * @file ConfigValue.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_CONFIGVALUE_HPP_
#define TPGLIBS_CONFIGVALUE_HPP_

#include <string>


namespace tpglibs {

template <typename T>
class ConfigValue : public GenericConfigValue {
  public:
    ConfigValue(T value) : m_config_value(value)
    {}

    T get_config_value() {
      return m_config_value;
    }

  private:
    T m_config_value;
};

} // namespace tpglibs

#endif // TPGLIBS_CONFIGVALUE_HPP_
