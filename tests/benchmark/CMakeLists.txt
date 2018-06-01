MACRO (M_ADD_LIBRARY_BENCHMARK name)
    ADD_EXECUTABLE(
            ${name}
            ${ARGN}
            bench_common.cc
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
            bench_common.cc
    )
    TARGET_LINK_LIBRARIES(${name}
            benchmark)
    ADD_TEST(NAME ${name} COMMAND ${name})
ENDMACRO()

M_ADD_MODULE_BENCHMARK(chunk_lock_bench
        chunk_lock_bench.cc
        ${phosphor_SOURCE_DIR}/src/chunk_lock.cc)

M_ADD_MODULE_BENCHMARK(tracing_threading_bench_1
        tracing_threading_bench.cc
        ${phosphor_SOURCE_FILES})
set_target_properties(tracing_threading_bench_1 PROPERTIES COMPILE_DEFINITIONS "PHOSPHOR_CHUNK_PAGE_COUNT=1")

M_ADD_MODULE_BENCHMARK(tracing_threading_bench_4
        tracing_threading_bench.cc
        ${phosphor_SOURCE_FILES})
set_target_properties(tracing_threading_bench_4 PROPERTIES COMPILE_DEFINITIONS "PHOSPHOR_CHUNK_PAGE_COUNT=4")

M_ADD_MODULE_BENCHMARK(tracing_threading_bench_16
        tracing_threading_bench.cc
        ${phosphor_SOURCE_FILES})
set_target_properties(tracing_threading_bench_16 PROPERTIES COMPILE_DEFINITIONS "PHOSPHOR_CHUNK_PAGE_COUNT=16")

M_ADD_LIBRARY_BENCHMARK(category_onoff_bench
        category_onoff_bench.cc)

M_ADD_LIBRARY_BENCHMARK(tracing_onoff_bench
        tracing_onoff_bench.cc)

M_ADD_LIBRARY_BENCHMARK(chunk_replacement_bench
        chunk_replacement_bench.cc)

M_ADD_LIBRARY_BENCHMARK(category_registry_bench
        category_registry_bench.cc)
