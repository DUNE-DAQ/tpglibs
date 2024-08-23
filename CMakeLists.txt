cmake_minimum_required(VERSION 3.12)
project(tpglibs VERSION 0.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-O2)

set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/cmake)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

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

daq_add_python_bindings(*.cpp LINK_LIBRARIES ${PROJECT_NAME})

daq_add_unit_test(avx_factory_test LINK_LIBRARIES tpglibs)
daq_add_unit_test(avx_generator_test LINK_LIBRARIES tpglibs)
daq_add_unit_test(avx_pipeline_test LINK_LIBRARIES tpglibs)
daq_add_unit_test(avx_processors_test LINK_LIBRARIES tpglibs)

daq_install()