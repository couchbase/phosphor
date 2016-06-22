MACRO (M_ADD_LIBRARY_GTEST name)
    ADD_EXECUTABLE(
            ${name}
            ${ARGN}
    )
    TARGET_LINK_LIBRARIES(${name}
            gmock
            gtest
            gtest_main
            phosphor)
    ADD_TEST(NAME ${name} COMMAND ${name})
ENDMACRO()

IF(DEFINED gtest_SOURCE_DIR)
    INCLUDE_DIRECTORIES(AFTER
            ${gtest_SOURCE_DIR}/include
            ${gmock_SOURCE_DIR}/include)
    ADD_SUBDIRECTORY(module)
    ADD_SUBDIRECTORY(library)
ENDIF ()

option(PHOSPHOR_ENABLE_BENCHMARKING "Enable benchmarking of Phosphor." OFF)
IF(DEFINED benchmark_SOURCE_DIR AND PHOSPHOR_ENABLE_BENCHMARKING)
    INCLUDE_DIRECTORIES(AFTER
            ${benchmark_SOURCE_DIR}/include)
    ADD_SUBDIRECTORY(benchmark)
ENDIF ()

ADD_EXECUTABLE(playground playground.cc)
TARGET_LINK_LIBRARIES(playground phosphor)
