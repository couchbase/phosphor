# Couchbase Event Tracing Functional Requirements

*Date: 2016-04-26*

## Introduction
This document details the functional requirements for the proposed Couchbase
Event Tracing API outlined in the Couchbase Event Tracing One Pager.

At a high level, an Application will make use of the proposed event tracing
Library to record events of interest, via the Tracing API. These events will
then be exported to a format compatible with the Chrome Event Viewer, so they
can be viewed and analysed by users.

## Event Recording
### Minimal Functional Requirements
The following requirements are considered to be the minimal set for the event
tracing library to record events.

Where performance metrics are expressed, these are assumed to be measured on a
modern (SandyBridge upwards ) 64bit Intel processor running at least 1.9GHz.

M.1: ✓ The API will allow the Application to record application-defined events
       by making an appropriate Tracing API call at from code.

M.2: ✓ An Event consists of (at a minimum):
- A timestamp
- A name
- A category
- A thread ID

M.3: ✓ Event timestamps must have precision of at least 1 microsecond. (Chromium
       JSON format only supports microsecond resolution)

M.4: ✓ Events can optionally have up to two user-defined 8Byte fields associated
       with them (in addition to the name, category and timestamp. These fields
       will have a limited set of types. Exact types TBD, but for example: bool,
       int, float, string, pointer.

M.5: ✓ Event Tracing can be enabled / disabled globally. When event tracing is
       globally disabled, the CPU overhead should be minimal (i.e. of the order
       of ASM-level load, compare & branch instructions).

M.6: ✓ The Library must record events into a RAM data structure (Event Log), and
       allow retrieval of all recorded events.

M.7: ✓ The size of the Event Log must be Application-configurable. It must at
       least allow a range of 1MB to 1GB.

M.8: ✓ The amount of RAM used by the Event Tracer must not exceed 110% of the
       configured log size when event tracing is enabled.

M.9: ✓ The event log can be cleared (all events discarded) when tracing is not
       running.

M.10: ✓ When the event log is empty (after clearing, or before it is enabled)
        the amount of RAM used by the Event Tracer must be less than 100KB.

M.11: ✓ When tracing is enabled, the cost of recording of a single event with
two user-defined fields to the in-memory Event Log (measured from before calling
the Event API to it returning) must be 1 microsecond or less.

M.12: ✗ The Library must facilitate the use of a memory-mapped file for the
Event Log (so the log can be directly persisted to disk by the OS). Note: The
Library does not need to handle the disk writing itself.

### Extended Functional Requirements
The following requirements describe desirable functionality, which increases the
utility and functionality of the event tracing feature - they are nice to have,
although the event tracing library.

E.1: ✓ Events can be enabled / disabled at a category level.

E.2: ✗ Events can be retrieved at a category level.

E.3: ✗ Events can be retrieved by specifying a timestamp start / end (i.e.
       "give me all events which fall between these two timestamps").

## Event Analysis
### Minimal Functional Requirements
The following requirements are considered the minimum for the analysis
component, which allows the recorded events to be viewed and analyzed by users.

M.100: ✓ The event log can be exported to a format which can be read by the
         Chrome Event Viewer (chrome://tracing).
