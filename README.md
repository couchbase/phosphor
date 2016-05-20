# Phosphor

[![Build Status](https://travis-ci.org/couchbaselabs/phosphor.svg?branch=master)](https://travis-ci.org/couchbaselabs/phosphor)
[![Coverage Status](https://coveralls.io/repos/github/couchbaselabs/phosphor/badge.svg?branch=master)](https://coveralls.io/github/couchbaselabs/phosphor?branch=master)
[![License](https://img.shields.io/github/license/couchbaselabs/phosphor.svg)](LICENSE)

Phosphor is a high-performance event tracing framework for C++11 applications
designed for use in Couchbase Server - specifically KV Engine and ForestDB.

Event tracing is implemented in an application by instrumenting it as
described in [phosphor.h](include/phosphor/phosphor.h). You can then enable
and manage tracing using the management API described in
[trace_log.h](include/phosphor/trace_log.h).

## Example
The following is an example of hypothetical instrumentation in memcached:

    void performSet(ENGINE_HANDLE* engine, const char* key, const char* value) {
        TRACE_EVENT_START("Memcached:Operation", "performSet", key)
        // Perform a set operation
        TRACE_EVENT_END0("Memcached:Operation", "performSet")
    }

The following is an example of enabling tracing with a fixed-style 5MB buffer:


    TraceLog::getInstance().start(TraceConfig(BufferMode::fixed, 5))
    
    
## Documentation

Phosphor has documentation for both internal and external use that can be
generated using Doxygen in the usual way.
