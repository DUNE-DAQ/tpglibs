/**
 * @file module.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "registrators.hpp"

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

namespace py = pybind11;

namespace dunedaq::tpglibs::python {

PYBIND11_MODULE(_daq_tpglibs_py, m)
{

  m.doc() = "C++ implementation of the tpglibs modules";

  // You'd want to change renameme to the name of a function which
  // you'd like to have a python binding to

  register_renameme(m);

  // Add attributes signaling if state monitoring is enabled, to be consumed by 
  // integration test tpg_state_collection_test
  
  #ifdef TPGLIBS_ENABLE_STATE_MONITORING
    m.attr("state_monitoring_enabled") = true;
  #else
    m.attr("state_monitoring_enabled") = false;
  #endif
}

} // namespace dunedaq::tpglibs::python
