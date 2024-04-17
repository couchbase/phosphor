macro(M_ADD_MODULE_BENCHMARK name)
    cb_add_test_executable(${name} ${ARGN} bench_common.cc)
    target_link_libraries(${name} benchmark::benchmark benchmark::benchmark_main)
    target_include_directories(${name} PRIVATE
            ${phosphor_SOURCE_DIR}/include
            ${phosphor_SOURCE_DIR}/thirdparty/dvyukov/include
            ${phosphor_SOURCE_DIR}/src
            ${phosphor_SOURCE_DIR}/tests/include)
    add_test(NAME ${name} COMMAND ${name})
endmacro()

m_add_module_benchmark(tracing_threading_bench_1
        tracing_threading_bench.cc
        ${phosphor_SOURCE_FILES})
set_target_properties(tracing_threading_bench_1 PROPERTIES COMPILE_DEFINITIONS "PHOSPHOR_CHUNK_PAGE_COUNT=1")

m_add_module_benchmark(tracing_threading_bench_4
        tracing_threading_bench.cc
        ${phosphor_SOURCE_FILES})
set_target_properties(tracing_threading_bench_4 PROPERTIES COMPILE_DEFINITIONS "PHOSPHOR_CHUNK_PAGE_COUNT=4")

m_add_module_benchmark(tracing_threading_bench_16
        tracing_threading_bench.cc
        ${phosphor_SOURCE_FILES})
set_target_properties(tracing_threading_bench_16 PROPERTIES COMPILE_DEFINITIONS "PHOSPHOR_CHUNK_PAGE_COUNT=16")

cb_add_test_executable(phosphor_benchmarks
        bench_common.cc
        chunk_lock_bench.cc
        category_onoff_bench.cc
        tracing_onoff_bench.cc
        chunk_replacement_bench.cc
        category_registry_bench.cc
)
target_link_libraries(phosphor_benchmarks PRIVATE
        benchmark::benchmark benchmark::benchmark_main phosphor)
target_include_directories(phosphor_benchmarks PRIVATE
        ${phosphor_SOURCE_DIR}/src
        ${phosphor_SOURCE_DIR}/tests/include)
add_test(NAME phosphor_benchmarks COMMAND phosphor_benchmarks)