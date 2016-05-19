add_executable(playground playground.cc)
target_link_libraries(playground event_tracer)

add_executable(sentinel sentinel.cc)