add_executable(test_avx_processors avx_processors.cxx)
target_link_libraries(test_avx_processors PRIVATE tpglibs fmt::fmt)
target_include_directories(test_avx_processors PRIVATE ${BOOST_INCLUDE_DIRS})
add_test(NAME avx_processors COMMAND test_avx_processors)

add_executable(test_avx_factory avx_factory.cxx)
target_link_libraries(test_avx_factory PRIVATE tpglibs fmt::fmt)
target_include_directories(test_avx_factory PRIVATE ${BOOST_INCLUDE_DIRS})
add_test(NAME avx_factory COMMAND test_avx_factory)

add_executable(test_avx_pipeline avx_pipeline.cxx)
target_link_libraries(test_avx_pipeline PRIVATE tpglibs fmt::fmt trgdataformats::trgdataformats)
target_include_directories(test_avx_pipeline PRIVATE ${BOOST_INCLUDE_DIRS})
add_test(NAME avx_pipeline COMMAND test_avx_pipeline)

add_executable(test_avx_generator avx_generator.cxx)
target_link_libraries(test_avx_generator PRIVATE tpglibs fmt::fmt fddetdataformats::fddetdataformats trgdataformats::trgdataformats)
target_include_directories(test_avx_generator PRIVATE ${BOOST_INCLUDE_DIRS})
add_test(NAME avx_generator COMMAND test_avx_generator)
