MACRO (M_ADD_BENCHMARK name)
    ADD_EXECUTABLE(
            ${name}
            ${ARGN}
    )
    TARGET_LINK_LIBRARIES(${name}
            benchmark)
    ADD_TEST(NAME ${name} COMMAND ${name})
ENDMACRO()

M_ADD_BENCHMARK(sentinel_bench
        sentinel_bench.cc
        ${phosphor_SOURCE_DIR}/src/sentinel.cc)
