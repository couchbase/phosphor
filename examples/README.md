# Phosphor Examples

This directory contains examples of Phosphor being used for tracing. The
examples in this directory can be built with the rest of the examples by using
the "-DPHOSPHOR_BUILD_EXAMPLES=1" CMake config flag or running `make all` with
the top-level-makefile (Which will also build the benchmarks, with coverage and
run the tests).

## sorting_algorithm

The sorting algorithm executable contains an implementation of merge sort and
will print out an array of ints before and after sorting. You can enable tracing
with Phosphor by using the PHOSPHOR_TRACING_START environment variable. e.g.

    PHOSPHOR_TRACING_START="buffer-mode:fixed,buffer-size:1241024,save-on-stop:sorting_output.%d.json" build/examples/sorting_algorithm

Will start tracing when the trace log object is instantiated (Usually at the
first call site) with a fixed size buffer roughly 1MB in size. It will also
automaticallted dump a file named along the lines of
`sorting_output.2016-06-21T08:00:45Z.json` when the trace log is destructed (On
program exit). This tracing file can be viewed in the Catapult tracing viewer
which is bundled with Chrome/Chromium (chrome://tracing/).