# One-pager: Tracing in KV-Engine
*Date: 2016-04-19*

*Author: Will Gardner*

The purpose of this document is to discuss general requirements and motivation
for an event tracing system in KV-Engine rather than implementation/API details.

## Motivation
An event tracer in KV-Engine is principally aimed at identifying performance
issues that a sampling profiler might not be able to trivially shed light on.
Examples include >99% operation latencies (such as those improved by DCP changes
in 3.1.4) and the NONIO thread being blocked by a non-yielding task (e.g. the
DCP consumer).

A well-tooled event tracer would allow for analysis both on a per-thread basis
and also on a request/command basis even across threads (Such as the transition
between front-end memcached and back-end ep-engine threads). This kind of
analysis would allow for identifying what makes an individual request slow or an
individual thread busy.

## What
An event tracer would maintain a timestamped log of various synchronous events
that occur in a single thread such as function calls (both enter and return) and
also asynchronous events that may occur across threads such as EWOULDBLOCK
handling. The event log would include contextual information such as thread id
and process id while asynchronous events would also include further event
identification such as the request cookie. In addition events would be taggable
with miscellaneous information such as function arguments.

Ideally the event tracer would be continually running and a dump of an in-memory
buffer could be performed as required (e.g. when performing a cb_collectinfo).
This could be flushed to disk or similar. Alternatively if the overhead of event
tracing is high, tracing could be enabled on demand and run for X amount of time
or Y amount of space. This could provide valuable insight into what is hurting
performance in a live customer environment. Category-based event filtering could
allow for differing levels of logging at specific times (e.g. low-level tracing
most of the times, but increased for a cb_collectinfo or developer debugging in
certain areas).

KV-Engine would be explicitly tooled with event logging in key areas via a
simple API.

## Inspiration
The Chromium Project’s event profiler
(https://www.chromium.org/developers/how-tos/trace-event-profiling-tool)
provides a lot of desirable functionality. Chromium’s event profiler is
comprised of two parts: C++ code tooling for generating traces, and a trace
viewer.

The code tooling is a simple macro-driven API
(https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/common/trace_event_common.h)
that stores traces to an in-memory buffer when triggered. The API is
minimalistic and where possible uses RAII to prevent repetition (e.g. function
entry/return). This buffer can be dumped to a JSON format
(https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/edit?pli=1)
when required.

The event viewer is embedded into Chrome (chrome://tracing/) and reads traces in
the aforementioned trace format. The code for the event viewer is stored
separately as part of the Catapult project
(https://github.com/catapult-project/catapult/tree/master/tracing) and could be
adapted to work with other browsers with minimal work.

There is also an alternative tracing framework (Also by Google)
http://google.github.io/tracing-framework/ that comes with some simple C++
tooling. The main advantage of this is the improved UI interactions but does
not have proper support for asynchronous events.
