add_executable(test_avx_processors avx_processors.cxx)
target_link_libraries(test_avx_processors PRIVATE tpgengine fmt::fmt)
target_include_directories(test_avx_processors PRIVATE ${BOOST_INCLUDE_DIRS})
add_test(NAME avx_processors COMMAND test_avx_processors)
