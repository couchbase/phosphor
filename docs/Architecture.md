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
 - A ChunkLock

#### ChunkLock

The ChunkLock is used to guard access to a Chunk. It is conceptually similar to
a reader/writer lock that only allows a single reader. It has three states:

 * Unlocked
 * SlaveLocked, locked by the TraceLog::logEvent frontend
 * MasterLocked, locked by the TraceLog::evictThreads backend

There are two distinct lock states as front-end consumers via logEvent will not
be blocked by the `MasterLocked` state (it uses try_lock). The reason for this
is that if the ChunkLock has been locked via the `evictThreads` route then
tracing will be shut down and front-end consumers don't need to acquire the
lock.
