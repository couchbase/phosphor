MACRO (M_ADD_LIBRARY_BENCHMARK name)
    ADD_EXECUTABLE(
            ${name}
            ${ARGN}
    )
    TARGET_LINK_LIBRARIES(${name}
            benchmark
            phosphor)
    ADD_TEST(NAME ${name} COMMAND ${name})
ENDMACRO()

MACRO (M_ADD_MODULE_BENCHMARK name)
    ADD_EXECUTABLE(
            ${name}
            ${ARGN}
    )
    TARGET_LINK_LIBRARIES(${name}
            benchmark)
    ADD_TEST(NAME ${name} COMMAND ${name})
ENDMACRO()

M_ADD_MODULE_BENCHMARK(sentinel_bench
        sentinel_bench.cc
        ${phosphor_SOURCE_DIR}/src/sentinel.cc)

M_ADD_LIBRARY_BENCHMARK(tracing_threading_bench
        tracing_threading_bench.cc)

M_ADD_LIBRARY_BENCHMARK(tracing_onoff_bench
        tracing_onoff_bench.cc)
