cmake_minimum_required(VERSION 3.12)
project(tpgengine VERSION 0.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-O2)

set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/cmake)

find_package(daq-cmake REQUIRED)
find_package(fmt REQUIRED)

find_package(nlohmann_json REQUIRED)
find_package(trgdataformats REQUIRED)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)

##############################################################################

set(FDREADOUTLIBS_USE_INTRINSICS ON)

if(${FDREADOUTLIBS_USE_INTRINSICS})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
endif()

##############################################################################

add_library(tpgengine SHARED
	src/AVXAbsRunSumProcessor.cpp
	src/AVXFrugalPedestalSubtractProcessor.cpp
	src/AVXRunSumProcessor.cpp
	src/AVXThresholdProcessor.cpp
	src/NaiveAbsRunSumProcessor.cpp
	src/NaiveFrugalPedestalSubstractProcessor.cpp
	src/NaiveRunSumProcessor.cpp
	src/NaiveThresholdProcessor.cpp
	src/AVXGenerator.cpp
	)
target_include_directories(tpgengine PUBLIC
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
	$<BUILD_INTERFACE:${PROJECTSOURCE_DIR}/src>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	)
target_link_libraries(tpgengine PRIVATE nlohmann_json::nlohmann_json trgdataformats::trgdataformats fmt::fmt)
install(TARGETS tpgengine EXPORT tpgengineTargets)

CONFIGURE_PACKAGE_CONFIG_FILE(cmake/tpgengineConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/tpgengineConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
	)

add_subdirectory(unittest)

install(EXPORT tpgengineTargets DESTINATION ${CMAKE_INSTALL_CMAKEDIR} NAMESPACE tpgengine::)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tpgengineConfig.cmake
	DESTINATION ${CMAKE_INSTALL_CMAKEDIR})
install(DIRECTORY include/${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.h??")
