add_library(phosphor_test_main OBJECT test_main.cc)
target_link_libraries(phosphor_test_main PRIVATE GTest::gtest)
set_target_properties(phosphor_test_main PROPERTIES EXCLUDE_FROM_ALL TRUE)

macro (M_ADD_LIBRARY_GTEST name)
    cb_add_test_executable(
            ${name}
            $<TARGET_OBJECTS:phosphor_test_main>
            ${ARGN}
    )
    target_link_libraries(${name}
            GTest::gmock
            GTest::gtest
            phosphor
            platform
            nlohmann_json::nlohmann_json)
    add_test(NAME ${name} COMMAND ${name})
endmacro()

include_directories(AFTER
        ${phosphor_SOURCE_DIR}/include
        ${phosphor_SOURCE_DIR}/src
        ${phosphor_SOURCE_DIR}/thirdparty/dvyukov/include
        ${phosphor_SOURCE_DIR}/tests/include)

add_subdirectory(module)
add_subdirectory(library)
add_subdirectory(benchmark)
