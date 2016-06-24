# Phosphor Architecture

## Frontend

The frontend consists of two main interfaces:

- Instrumentation Macros
- TraceLog / TraceLogConfig / TraceConfig

### Instrumentation Macros

The instrumentation macros are basically wrappers around the TraceLog::logEvent
member functions. There are also some additional utilities, such as the scoped
macros.

### TraceLog

The TraceLog is the primary interface used for interacting with Phosphor
externally. It is used by the instrumentation macros for actually logging events
and also used directly by end users for configuration etc.

The TraceLog encapsulates most of the logic for actually managing tracing
tracing events and tying all the parts of the system together.

#### TraceLogConfig

The TraceLogConfig class is used to perform one-time configuration of a
TraceLog and consists of configuration that can only performed once.

#### TraceConfig

The TraceConfig class is used to perform per-trace configuration of a TraceLog
and is passed directly to the TraceLog::start member function.

## Backend

### TraceEvent

The TraceEvent class represents a singular event which contains a name,
category, timestamp (Which is accurate/meaningful relative to other events) and
optional arguments along with the type for those arguments.

### TraceChunk

A TraceChunk is conceptually a collection of TraceEvents and contains 1 or more
pages worth of TraceEvents.

### TraceBuffer

The TraceBuffer is a collection of TraceChunks. It loans TraceChunks out to
ChunkTenants.

### ChunkTenant

A ChunkTenant is conceptually an object which borrows a TraceChunk from a
TraceBuffer. A ChunkTenant can either be allocated per-thread (In the event that
threads have been registered appropriately) or can be shared using a pool of
ChunkTenants distributed by thread id. A ChunkTenant has two main parts

 - A pointer to the currently borrowed chunk (Or nullptr if not currently
   borrowing one).
 - A chunk sentinel

#### Chunk `Sentinel`

The sentinel is used to guard access to a Chunk. It is principally a tri-state
lock:

 * Open: the chunk is currently unlocked and the current pointer is either null
         or it is safe to use
 * Busy: the chunk is currently locked and the pointer is either safe to use or
         being made safe by the person who locked the sentinel.
 * Closed: the chunk is currently unlocked but the current pointer is not safe
           to use. A TraceLog transitions a sentinel into this third state to
           indicate that the next user of the ChunkTenant should replace or
           at least nullify the chunk.

The TraceLog will transition the chunk sentinel to closed when the trace buffer
gets full or when tracing is stopped, this is usually because the trace buffer
ownership will be transferred away and the chunk 'loans' need to be revoked.
This transition can only be done when the sentinel is open and will block until
it is in this state.

A thread using a ChunkTenant will try to transition its sentinel to busy when
they need to log an event and back to open when it's done. It will fail to do
this if the sentinel is currently closed. In this event the chunk tenant will
transition the sentinel to busy in another way and be aware that it needs to
replace the chunk it had before.

