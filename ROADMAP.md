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
- <strike>Add platform abstraction for thread ids</strike> Done 2016/06/06
- <strike>Switch to simpler time representation</strike> Done 2016/06/06
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
- Reduce size of trace event by reducing to 24 bits of microseconds (~16 seconds
from 2^64 nanoseconds and storing a further 16 bits of resolution in the chunk
header. (Saves ~4 bytes since type can be combined into timestamp)
- Reduce size of trace event by combining trace category and trace name pointers
(Saves 8 bytes as its one less pointer)
- Assume one less argument on Async events and use id as one of the arguments
(Saves 8 bytes as there's no need for separate id property)
- Add a duration property (Costs 24 bits)

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

## Candidate events tracing locations

* memcached
  - libevent callbacks being invoked (look for calls to `event_assign` / `event_add` for where callbacks are registered). Arguments: `fd`, `which` and the void* `argument` ideally, although I belive only 2 are currently supported.
  - Handling of a binary protocol request (i.e. equivilent to what mctimings currently measures). See `mcbp_collect_timings`. Arguments: Will be hard/costly to record much of most requests, but for example where there's a finite set (e.g. `stats` command sub-command) we should record them.
  - statemachine changes for connections (`conn_XXX` functions). Note these will be very frequent so probably want a explicit trace for each function (`conn_new_cmd`, `conn_nread` etc) as the cost of calling getTaskName each time may be costly.
  - engine API functions (function pointers defined in _engine.h_, wrapper functions for common cases in _memcached.h_ (`bucket_get`, `bucket_store` etc). We probably need to expand the wrappers so all engine API functions are traced - see for example `add_set_replace_executor` which directly calls `allocate` method. Arguments:Specifically of interest will be any functions returning EWOULDBLOCK - and recording when that happens.
  
* ep-engine
  - Acquire / release of (expensive) locks. Examples: `CheckpointManager::queueLock`
  - VBucket state changes (active/replica/dead).
  - DCP
    - Stream start (Arguments: vbucket, start, end)
    - Stream end (Arguments: reason for finish).
    - Event per checkpoint marker? (if not too expensive).
  - KVStore
    - committing a new checkpoint (`KVStore::commit`)
    - rollback
    - compaction
    - snapshotVBucket
