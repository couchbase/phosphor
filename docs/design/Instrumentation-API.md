# Event Tracing Instrumentation API Spec
*Date: 2016-04-19*

*Author: Will Gardner*

**Recommended Reading:**

* [One-pager](./One-pager.md)

* Chromium Project event tracing API:
[https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/common/trace_event_common.h](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/common/trace_event_common.h&q=f:trace_event_common.h)

* * *

## Outline:

The purpose of this spec is to specify the user API for tooling a C++
application for tracing. The user API will consist of preprocessor macros. The
reasoning behind this is twofold, firstly it allows for for trivially compiling
out the tracing (e.g. for production or testing performance overhead), secondly
it makes it easier to start instrumenting code while the tracing implementation
is under development.

This document does not detail the API for initialising tracing or triggering
some kind of tracing dump.

## Event Types

There will be initially three event types:

* Instant (ie. no duration)
* Synchronous (start, stop on same thread and is in theory executing for that
               entire time - useful for tracing execution time of a function)
* Asynchronous (start, stop on any thread and may or may not be actively
                executed - useful for things like EWOULDBLOCK tracing)

## Macro Examples

Every event has common parameters of category, event_name and, optionally,
arguments. Both category and event_name should be fixed cstrings (ie. won’t go
out of scope) - this allows for potential optimisations (e.g. just store the
pointer). The category might be just a single category in an initial
implementation with a comma separated list in future.

### Synchronous (entry and return)

    TRACE_EVENT(category, event_name, args...)

In this example a start event `event_name` is logged to `category` with a
variadic number of `args`. Thread ID and the time are automatically collected.
This will also place a variable in the current scope that will log a
corresponding end event when it goes out of scope. This might be useful for
logging the start and end of an expensive function e.g.

    fdb_status fdb_get_kv(fdb_kvs_handle *handle,
                          const void *key,
                          void **value_out) {
        /* Assuming the disk category is enabled, log the fdb_get_kv
           start event with the handle and the key requested. */
        TRACE_EVENT("disk", "fdb_get_kv", handle, key);

        /* Do something expensive like fetch from disk */
        ...

        /* Automatically add end event for the function call */
        return status;
    }

### Synchronous (Explicit)

    TRACE_EVENT_START(category, event_name, args...)
    TRACE_EVENT_END(category, event_name)

This example is more or less identical to the previous, except the end of the
trace event is explicit. This must be called from a single thread as the start
and end events will be matched up even when nested. This can be useful for
tracing an event that occurs across functions or in the middle of a function,
e.g. acquiring an expensive/global lock:

    DcpResponse* DcpProducer::getNextItem() {
        TRACE_EVENT_START("dcp", "DcpProducer::getNextItem()/get-expensive-lock");
        /* Acquire an expensive lock */
        TRACE_EVENT_END("dcp", "DcpProducer::getNextItem()/get-expensive-lock");

        /* Do stuff with the lock */
    }

### Instant

    TRACE_EVENT_INSTANT(category, event_name, args...)

In this example a single event will be logged that is effectively equivalent to
the synchronous start event, except without an expectation of a corresponding
end event.

    void HashTable::remove(Item* itm) {
        /* Inexpensive function call but worth logging */
        TRACE_EVENT_INSTANT("hashtable", "HashTable::remove", itm->key);
    }

### Asynchronous

    TRACE_ASYNC_EVENT_START(category, event_name, event_id, args...)
    TRACE_ASYNC_EVENT_END(category, event_name, event_id)

The primary difference with asynchronous events is that they can occur across
threads and out of order which means matching them cannot be inferred, therefore
they have an event_id for tracking this. The event_id would be some kind of
opaque integer/pointer. Examples include EWOULDBLOCK handling:

    /* On a front-end memcached thread */
    ENGINE_ERROR_CODE EvpStore(ENGINE_HANDLE* handle,
                               const void *cookie,
                               item* itm) {

        TRACE_ASYNC_EVENT_START('persist', 'EvpStore/persistTo',
                                cookie, itm->key);
        /* Add to disk-write queue */

    }


    /* On a writer thread */
    void PersistenceCallback::callback(mutation_result &value) {
        TRACE_ASYNC_EVENT_END("persist", "EvpStore/persistTo",
                              queued_value->cookie);
        /* Notify memcached */
    }
