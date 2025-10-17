/**
 * @file Types.hpp Common types in tpglibs
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_TYPES_HPP_
#define TPGLIBS_TYPES_HPP_

#include "tpglibs/GenericConfigValue.hpp"

#include <memory>
#include <string>
#include <unordered_map>


namespace tpglibs {
namespace types {

using tpg_config_map_t = std::unordered_map<std::string, std::shared_ptr<GenericConfigValue>>;

} // namespace types
} // namespace tpglibs

#endif // TPGLIBS_TYPES_HPP_
