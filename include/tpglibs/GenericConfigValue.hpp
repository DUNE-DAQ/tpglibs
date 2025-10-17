/**
 * @file GenericConfigValue.hpp
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_GENERICCONFIGVALUE_HPP_
#define TPGLIBS_GENERICCONFIGVALUE_HPP_

#include <string>

namespace tpglibs {

struct GenericConfigValue {
  std::string config_name {""};
};

} // namespace tpglibs

#endif // TPG_GENERICCONFIGVALUE_HPP_
