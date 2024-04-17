add_library(phosphor_test_main OBJECT test_main.cc)
target_link_libraries(phosphor_test_main PRIVATE GTest::gtest)
set_target_properties(phosphor_test_main PROPERTIES EXCLUDE_FROM_ALL TRUE)

add_subdirectory(module)
add_subdirectory(library)
add_subdirectory(benchmark)
