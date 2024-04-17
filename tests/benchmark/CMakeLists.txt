macro (M_ADD_LIBRARY_BENCHMARK name)
    cb_add_test_executable(${name} ${ARGN} bench_common.cc)
    target_link_libraries(${name} benchmark::benchmark benchmark::benchmark_main phosphor)
    add_test(NAME ${name} COMMAND ${name})
endmacro()

macro (M_ADD_MODULE_BENCHMARK name)
    cb_add_test_executable(${name} ${ARGN} bench_common.cc)
    target_link_libraries(${name} benchmark::benchmark benchmark::benchmark_main)
    add_test(NAME ${name} COMMAND ${name})
endmacro()

m_add_module_benchmark(chunk_lock_bench
        chunk_lock_bench.cc
        ${phosphor_SOURCE_DIR}/src/chunk_lock.cc)

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

m_add_library_benchmark(category_onoff_bench
        category_onoff_bench.cc)

m_add_library_benchmark(tracing_onoff_bench
        tracing_onoff_bench.cc)

m_add_library_benchmark(chunk_replacement_bench
        chunk_replacement_bench.cc)

m_add_library_benchmark(category_registry_bench
        category_registry_bench.cc)
