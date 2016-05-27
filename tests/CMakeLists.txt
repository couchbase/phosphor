IF(DEFINED gtest_SOURCE_DIR)
    INCLUDE_DIRECTORIES(AFTER
            ${gtest_SOURCE_DIR}/include
            ${gmock_SOURCE_DIR}/include)
    ADD_SUBDIRECTORY(module)
ENDIF ()

option(PHOSPHOR_ENABLE_BENCHMARKING "Enable benchmarking of Phosphor." OFF)
IF(DEFINED benchmark_SOURCE_DIR AND PHOSPHOR_ENABLE_BENCHMARKING)
    INCLUDE_DIRECTORIES(AFTER
            ${benchmark_SOURCE_DIR}/include)
    ADD_SUBDIRECTORY(benchmark)
ENDIF ()

ADD_EXECUTABLE(playground playground.cc)
TARGET_LINK_LIBRARIES(playground phosphor)
