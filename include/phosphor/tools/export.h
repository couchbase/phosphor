/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include "phosphor/trace_buffer.h"
#include "phosphor/trace_context.h"
#include "phosphor/trace_log.h"

namespace phosphor {
namespace tools {

/**
 * The JSONExport class is a tool provided to allow exporting
 * a TraceBuffer in the Chromium Tracing JSON format in a
 * chunked manner (e.g. for over the network).
 *
 * Usage:
 *
 *     auto buffer = phosphor::TraceLog::instance().getBuffer();
 *     JSONExport exporter(*buffer);
 *
 *     do {
 *         p = exporter.read(80);
 *         std::cerr << p;
 *     }  while(p.size());
 *
 * The above will write the JSON to stderr.
 *
 */
class JSONExport {
public:
    /**
     * Creates the export object
     */
    JSONExport(const TraceContext& _context);

    ~JSONExport();

    /**
     * Read 'length' worth of JSON
     *
     * @param out roughly 'length' bytes of the JSON starting from
     *            the point that was previously left off. This will
     *            return less than 'length' at the end of the buffer.
     * @param length Max size in bytes of the JSON to put into out
     * @return Number of bytes written to out
     */
    size_t read(char* out, size_t length);

    /**
     * Read 'length' worth of JSON
     *
     * @returns roughly 'length' bytes of the JSON starting from
     *          the point that was previously left off. This will
     *          return less than 'length' at the end of the buffer.
     */
    std::string read(size_t length);

    /**
     * Read entire buffer's worth of JSON
     *
     * @returns The entire buffer converted to JSON
     */
    std::string read();

    /**
     * @return True if the export is complete
     */
    bool done();

protected:
    enum class State {
        opening,
        /* first_event is only used if first_thread wasn't */
        first_event,
        other_events,
        first_thread,
        other_threads,
        footer,
        dead
    };

    const TraceContext& context;
    TraceBuffer::event_iterator it;
    std::unordered_map<uint64_t, std::string>::const_iterator tit;

    State state = State::opening;
    std::string cache;
};

/**
 * Reference callback for saving a buffer to a file if tracing stops.
 *
 * This saves the buffer to file in the same thread that it is called
 * from so that it may be used after main has returned (e.g. to write
 * everything that a given process has traced when the global TraceLog
 * is destructed).
 */
class FileStopCallback : public TracingStoppedCallback {
public:
    /**
     * @param _file_path File path to save the buffer to on completion,
     *                   may accept the wild cards %p for PID and %d for
     *                   an ISOish timestamp 'YYYY.MM.DDTHH.MM.SS'
     */
    FileStopCallback(const std::string& _file_path = "phosphor.%p.json");

    ~FileStopCallback();

    /**
     * Callback method called by TraceLog
     *
     * @param log Reference to the calling TraceLog
     * @param lh The lock being held when this callback is invoked
     */
    void operator()(TraceLog& log, std::lock_guard<TraceLog>& lh) override;

    /// Exposed for testing
    std::string generateFilePath();

private:
    std::string file_path;
};
} // namespace tools
} // namespace phosphor
