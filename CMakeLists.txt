

# This is a skeleton CMakeLists.txt file, auto-generated on
# Thu Jul 18 08:08:08 CEST 2024.  The developer(s) of this package should delete
# this comment as well as adding dependent targets, packages,
# etc. specific to the package. For details on how to write a package,
# please see
# https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-cmake/

cmake_minimum_required(VERSION 3.12)
project(tpgengine VERSION 0.0.0)

find_package(daq-cmake REQUIRED)
find_package(fmt REQUIRED)

find_package(nlohmann_json REQUIRED)
find_package(fddetdataformats REQUIRED)
find_package(trgdataformats REQUIRED)

daq_setup_environment()


find_package(Boost COMPONENTS unit_test_framework REQUIRED)

##############################################################################


# See https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-cmake/#daq_add_library

daq_add_library( LINK_LIBRARIES ) # Any source files and/or dependent libraries to link in not yet determined

##############################################################################


# See https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-cmake/#daq_add_python_bindings


daq_add_python_bindings(*.cpp LINK_LIBRARIES ${PROJECT_NAME} ) # Any additional libraries to link in beyond the main library not yet determined


##############################################################################


# See https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-cmake/#daq_add_unit_test

daq_add_unit_test(Placeholder_test LINK_LIBRARIES)  # Placeholder_test should be replaced with real unit tests

##############################################################################

daq_install()

