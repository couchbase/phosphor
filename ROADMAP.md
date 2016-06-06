# Roadmap
The following is a rough road-map and a plan for which weeks they will be
accomplished.

### WEEK COMMENCING 2016/05/23

- <strike>Finish test coverage of trace buffer</strike> Done 2016/05/27
- <strike>Add tests for Trace log</strike> Done 2016/05/27

### WEEK COMMENCING 2016/05/30

n/a @will.gardner is away

### WEEK COMMENCING 2016/06/06

- <strike>Add string conversion for TraceEvent::Type</strike> Done 2016/06/06
- Add platform abstraction for thread ids
- Add a circular buffer implementation
- Refactor TraceLog::logEvent to not duplicate code
- Add chunked JSON export (Which takes an iterator)
- Add benchmarking for chunk sizes (By templating the TraceChunk on the
base type of the chunk itself, ie. std::array vs gsl_p::dyn_array)
- Add benchmarking for tracing speed

### WEEK COMMENCING 2016/06/13

- Make TraceChunk/TraceEvent/TraceArgument a trivial type (If possible) to
avoid pre-allocation.
- Move TraceChunk constructor to a reset method
- Add binary dump export (Including tool to convert binary dump to JSON)
  - Write buffer to file
  - Write thread / pointer info to file

## Performance Idea List

For performance improvements that have been suggested or considered but are
currently premature:

- Check enabled flag with relaxed atomic
- Try avoiding the conditional branch caused by the ternary expression in
TraceLog::logEvent
- Use \__builtin_expect where useful
- Sentinel benchmarking suggests that double the number of logical cores
is the sweet-spot for the size of a shared array of ChunkTenants.

## MMap compatibility

Since the trace buffer has been refactored to be contiguous again an MMap is
technically possible. By switching to std::function based buffer factories it
is possible for an end-user (Or even a sample implementation) to create a buffer
which uses placement-new to allocate into a buffer. 

Additional stuff to make this practical:

 - Add allocator to support dyn_array
 - Create an MMap platform abstraction
 - Create an MMap based buffer with a factory-factory for specifying filename
 - Add callback for new events / categories to write their pointers/values to
 an info file at runtime.
