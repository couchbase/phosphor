/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#pragma once

#include <memory>
#include <mutex>

#include "trace_buffer.h"

namespace phosphor {

    // Forward declare
    class PHOSPHOR_API TraceLog;
    class PHOSPHOR_API TraceConfig;

    /**
     * Functor type for a callback to be used when a TraceLog stops
     * tracing. This can be given to a TraceConfig before tracing
     * starts in order to detect when it ends.
     *
     * The functor will receive a reference to the TraceLog and a
     * reference to the external lock that was held when the functor
     * was invoked which can be used to access restricted methods
     * even while the TraceLog is locked.
     *
     * Example Functor:
     *
     *     void(TraceLog& log, std::lock_guard<TraceLog>& lh) {
     *         for(const auto& event : *log.getBuffer(lh)) {
     *             std::cerr << event << "\n";
     *         }
     *         log.start(lh, TraceConfig(BufferMode::fixed, 2000000));
     *     }
     *
     * It is worth bearing in mind that this callback will be run in
     * the thread that stopped tracing which *could* be a thread which
     * was in the middle of tracing an event if the buffer became full.
     *
     * Therefore it might be sensible to create a new thread to process
     * the buffer (Which can take a comparatively long time) or to stash
     * it somewhere that another thread can access.
     */
    class TracingStoppedCallback {
    public:
        virtual ~TracingStoppedCallback() {}

        virtual void operator()(TraceLog&, std::lock_guard<TraceLog>&) = 0;

    };

    /**
     * The mode of a TraceBuffer implementation
     *
     *   - Custom mode signifies a custom implementation given by the user
     *   - Fixed mode uses a fixed amount of space and will get full
     *   - Ring mode never runs out of space as it will reuse old chunks
     */
    enum class BufferMode : char {
        custom = 0,
        fixed,
        ring,
    };

    /**
     * ostream operator overload for BufferMode
     */
    PHOSPHOR_API
    std::ostream& operator<<(std::ostream& stream, const BufferMode mode);

    /**
     * Custom deleter for our StringPtr class. Exists to create an
     * explicitly non-inline destructor.
     */
    struct PHOSPHOR_API StringPtrDeleter {
        void operator()(const std::string* ptr);
    };

    /**
     * String is used to expose string objects to clients, for example
     * when reading phosphor config strings.
     *
     * This is part of the DLL interface, and hence we need a type
     * which cannot be deleted directly by the client application -
     * phosphor itself must delete it to prevent mismatches between
     * allocate and free. Therefore we use a unique_ptr with a custom
     * (non-inline) deleter to ensure phosphor itself always deletes
     * any instances.
     */
    using StringPtr = std::unique_ptr<const std::string, StringPtrDeleter>;

    /**
     * make_unique equivilent for the StringPtr class.
     *
     * This should be used to contruct all instances of StringPtr, to ensure
     * matching alloc/free.
     */
    StringPtr make_String(const std::string& str);

    /**
     * The TraceLogConfig is used to perform a one-time config of a
     * TraceLog for anything that must be set only once e.g. the number
     * of shared sentinels to create.
     *
     * The TraceLogConfig can either be passed in when the TraceLog is
     * created, or by using the TraceLog::configure() method *prior* to
     * the first time the TraceLog is started.
     */
    class PHOSPHOR_API TraceLogConfig {
    public:
        /**
         * Default constructor establishes sensible default values for
         * one-time config.
         *
         *  Sentinel Count: x4 number of logical cores
         */
        TraceLogConfig();

        /**
         * Set the number of sentinels to create to be shared by
         * threads when they do not register.
         *
         * Example use:
         *
         *     TraceLogConfig()->setSentinelCount(48);
         *
         * @param _sentinel_count The number of sentinels
         * @return A reference to this config (For chaining)
         */
        TraceLogConfig& setSentinelCount(unsigned _sentinel_count);

        /**
         * @return The number of sentinels to be created
         */
        unsigned getSentinelCount() const;

        /**
         * Sets the TraceLog to start tracing immediately on construction
         * with a particular config
         *
         * @param _startup_trace A reference to a preexisting config that
         *                       will be copied for internal storage.
         * @return A reference to this config
         */
        TraceLogConfig& setStartupTrace(const TraceConfig& _startup_trace);

        /**
         * Clears the startup trace config that has previously been stored
         * so that tracing on startup can be disabled.
         *
         * @return A reference to this config
         */
        TraceLogConfig& clearStartupTrace();

        /**
         * @return a pointer to the TraceConfig (Because it is potentially null,
         *         i.e. tracing should not start)
         */
        TraceConfig* getStartupTrace() const;

        /**
         * Factory method which sets up a TraceLogConfig from the
         * environment variables
         *
         * Note: This is a member function because MSVC2012 can't do
         * copy elision properly.
         *
         * @param the TraceLogConfig to initialise
         */
        TraceLogConfig& fromEnvironment();

    protected:
        unsigned sentinel_count;
        std::unique_ptr<TraceConfig> startup_trace;
    };

    /**
     * The TraceConfig is used to configure a TraceLog for starting Trace
     * when it is enabled.
     *
     * The TraceConfig has two constructors with two different aims in mind
     *
     *   - Using a built-in TraceBuffer type, either ring or fixed
     *   - Using a user supplied TraceBuffer by supplying a TraceBufferFactory
     *
     * The first of these is specified by using the mode enumeration, the
     * second is specified by passing in the TraceBufferFactory.
     *
     * The second parameter to both of these is the size in bytes of the
     * TraceBuffer.
     *
     * All other arguments are optional and may be specified using chainable
     * methods.
     */
    class PHOSPHOR_API TraceConfig {
    public:
        TraceConfig();

        ~TraceConfig();

        /**
         * Constructor used when using a builtin TraceBuffer type
         *
         * @param _buffer_mode Which buffer mode to use. Cannot be
         *                    BufferMode::Custom.
         * @param _buffer_size Maximum size in bytes of the trace buffer.
         */
        TraceConfig(BufferMode _buffer_mode, size_t _buffer_size);

        /**
         * Constructor used when supplying a custom TraceBuffer implementation
         *
         * @param _buffer_factory The trace buffer factory to be used.
         * @param _buffer_size Maximum size in bytes of the trace buffer.
         */
        TraceConfig(trace_buffer_factory _buffer_factory, size_t _buffer_size);

        /**
         * @return The buffer mode that is selected
         */
        BufferMode getBufferMode() const;

        /**
         * @return The size of the buffer in megabytes that will be used
         */
        size_t getBufferSize() const;

        /**
         * @return The trace buffer factory that will be used to create a
         *         TraceBuffer when tracing is enabled.
         */
        trace_buffer_factory getBufferFactory() const;

        /**
         * Set the tracing_stopped_callback to be invoked when tracing
         * stops.
         *
         * @param _tracing_stopped_callback Callback to be used. Note
         *        this is passed as a shared_ptr due to it not being
         *        safe to copy Callbacks in the general case, as it
         *        may have been allocated with a different CRT than
         *        phosphor itself is linked against.
         * @return reference to the TraceConfig be configured
         */
        TraceConfig& setStoppedCallback(
            std::shared_ptr<TracingStoppedCallback> _tracing_stopped_callback);

        /**
         * @return The tracing_stopped_callback to be invoked when tracing
         * stops.
         */
        TracingStoppedCallback* getStoppedCallback() const;

        /**
         * Sets whether or not the tracing shutdown (and therefore callbacks)
         * should be run when the TraceLog is destroyed. Defaults to false.
         *
         * @param _stop_tracing Stop tracing on shutdown
         * @return reference to the TraceConfig being configured
         */
        TraceConfig& setStopTracingOnDestruct(bool _stop_tracing);

        /**
         * @return Whether or not the tracing shutdown (and therefore callbacks)
         *         should be run when the TraceLog is destroyed.
         */
        bool getStopTracingOnDestruct() const;

        /**
         * Set the categories to enable/disable in this trace config
         *
         * @param enabled The categories to explicitly enable
         * @param disabled The categories to explicitly disable
         * @return reference to the TraceConfig being configured
         */
        TraceConfig& setCategories(const std::vector<std::string>& enabled,
                                   const std::vector<std::string>& disabled);

        /**
         * @return The enabled categories for this trace config
         */
        const std::vector<std::string>& getEnabledCategories() const;

        /**
         * @return The disabled categories for this trace config
         */
        const std::vector<std::string>& getDisabledCategories() const;

        /**
         * Generate a TraceConfig from a config string (Usually set from
         * an environment variable).
         *
         * Example:
         *
         *     TraceConfig::fromString("buffer-mode:fixed,buffer-size:1024");
         *
         * @param config Config string to be used to generate the TraceConfig
         * @return Generated TraceConfig
         */
        static TraceConfig fromString(const std::string& config);

        /**
         * Converts a TraceConfig to a config string.
         *
         * This can be used to convert the TraceConfig that a TraceLog is
         * using to a human readable form.
         *
         * @return The config string
         */
        StringPtr toString() const;

    protected:
        /**
         * Get the trace buffer factory for the given mode.
         *
         * Cannot be used for the custom mode
         *
         * @param mode The trace buffer mode to convert
         * @return The trace buffer factory for
         * @throw std::invalid argument if given mode is invalid
         */
        static trace_buffer_factory modeToFactory(BufferMode mode);

        BufferMode buffer_mode = BufferMode::fixed;
        size_t buffer_size = 0;
        trace_buffer_factory buffer_factory = nullptr;
        std::shared_ptr<TracingStoppedCallback> tracing_stopped_callback;
        bool stop_tracing = false;

        std::vector<std::string> enabled_categories;
        std::vector<std::string> disabled_categories;
    };

}
