cmake_minimum_required(VERSION 3.12)
project(tpglibs VERSION 1.0.5)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

if(DEFINED ENV{TPGLIBS_ENABLE_STATE_MONITORING})
  option(TPGLIBS_ENABLE_STATE_MONITORING "Enable TPG internal state monitoring" $ENV{TPGLIBS_ENABLE_STATE_MONITORING})
else()
  option(TPGLIBS_ENABLE_STATE_MONITORING "Enable TPG internal state monitoring" OFF)
endif()

find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(detdataformats REQUIRED)
find_package(fddetdataformats REQUIRED)
find_package(trgdataformats REQUIRED)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)

##############################################################################

set(FDREADOUTLIBS_USE_INTRINSICS ON)

if(${FDREADOUTLIBS_USE_INTRINSICS})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
endif()

##############################################################################

daq_add_library( *.cpp  LINK_LIBRARIES nlohmann_json::nlohmann_json detdataformats::detdataformats fddetdataformats::fddetdataformats trgdataformats::trgdataformats fmt::fmt )

if(TPGLIBS_ENABLE_STATE_MONITORING)
  target_compile_definitions(tpglibs PUBLIC TPGLIBS_ENABLE_STATE_MONITORING)
endif()

daq_add_python_bindings(*.cpp)

daq_add_unit_test(avx_factory_test LINK_LIBRARIES tpglibs)
daq_add_unit_test(avx_generator_test LINK_LIBRARIES tpglibs)
daq_add_unit_test(avx_pipeline_test LINK_LIBRARIES tpglibs)
daq_add_unit_test(avx_processors_test LINK_LIBRARIES tpglibs)
if(TPGLIBS_ENABLE_STATE_MONITORING)
  daq_add_unit_test(ProcessorInternalStateNameRegistry_test LINK_LIBRARIES tpglibs)
  daq_add_unit_test(ProcessorInternalStateBufferManager_test LINK_LIBRARIES tpglibs)
  daq_add_unit_test(avx_processors_internal_state_collect_test LINK_LIBRARIES tpglibs)
  daq_add_unit_test(processor_state_registrate_collect_test LINK_LIBRARIES tpglibs)
endif()
daq_add_unit_test(binary_signal_reader_test LINK_LIBRARIES tpglibs)

# Add test application
daq_add_application(tpglibs_processor_test test_tpg_processor_app.cxx TEST LINK_LIBRARIES tpglibs)

daq_install()
