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

#include <cstring>
#include <exception>
#include <string>

#include "phosphor/tools/export.h"
#include "phosphor/trace_config.h"
#include "utils/memory.h"
#include "utils/string_utils.h"

namespace phosphor {

    std::ostream &operator<<(std::ostream &stream, const BufferMode mode) {
        switch (mode) {
            case BufferMode::custom:
                stream << "custom";
                break;
            case BufferMode::fixed:
                stream << "fixed";
                break;
            case BufferMode::ring:
                stream << "ring";
                break;
        }
        return stream;
    }

    void StringPtrDeleter::operator()(const std::string* ptr) {
        delete ptr;
    }

    StringPtr make_String(const std::string& str) {
        return StringPtr(new std::string(str));
    }

/*
 * TraceLogConfig implementation
 */
    TraceLogConfig &TraceLogConfig::setStartupTrace(
            const TraceConfig &_startup_trace) {
        startup_trace = utils::make_unique<TraceConfig>(_startup_trace);
        return *this;
    }

    TraceLogConfig &TraceLogConfig::clearStartupTrace() {
        startup_trace.reset();
        return *this;
    }

    TraceConfig *TraceLogConfig::getStartupTrace() const {
        return startup_trace.get();
    }

    TraceLogConfig &TraceLogConfig::fromEnvironment() {
        const char *startup_config = std::getenv("PHOSPHOR_TRACING_START");
        if (startup_config && strlen(startup_config)) {
            this->setStartupTrace(TraceConfig::fromString(startup_config));
        }

        return *this;
    }

/*
 * TraceConfig implementation
 */

    // Define the default constructor and destructor non-inline because this
    // class is part of the DLL interface and the executable may be using a
    // different CRT. Therefore by making the destructor non-inline we ensure
    // that memory is allocated and freed using the same runtime as the
    // constructors.
    TraceConfig::TraceConfig() = default;

    TraceConfig::~TraceConfig() = default;

    TraceConfig::TraceConfig(BufferMode _buffer_mode, size_t _buffer_size)
            : buffer_factory_container(_buffer_mode),
              buffer_size(_buffer_size),
              enabled_categories({{"*"}}) {}

    TraceConfig::TraceConfig(trace_buffer_factory _buffer_factory,
                             size_t _buffer_size)
            : buffer_factory_container(_buffer_factory),
              buffer_size(_buffer_size),
              enabled_categories({{"*"}}) {}

    trace_buffer_factory TraceConfig::BufferFactoryContainer::modeToFactory(BufferMode mode) {
        switch (mode) {
            case BufferMode::fixed:
                return trace_buffer_factory(make_fixed_buffer);
            case BufferMode::ring:
                return trace_buffer_factory(make_ring_buffer);
            case BufferMode::custom:
                throw std::invalid_argument(
                        "phosphor::TraceConfig::BufferFactoryContainer::modeToFactory: "
                        "Cannot get factory for Custom Mode");
        }
        throw std::invalid_argument(
                "phosphor::TraceConfig::BufferFactoryContainer::modeToFactory: "
                "Invalid buffer mode");
    }

    BufferMode TraceConfig::getBufferMode() const {
        return buffer_factory_container.mode;
    }

    trace_buffer_factory TraceConfig::getBufferFactory() const {
        return buffer_factory_container.factory;
    }

    size_t TraceConfig::getBufferSize() const {
        return buffer_size;
    }

    TraceConfig &TraceConfig::setStoppedCallback(
            std::shared_ptr<TracingStoppedCallback> _tracing_stopped_callback) {
        tracing_stopped_callback = _tracing_stopped_callback;
        return *this;
    }

    TracingStoppedCallback* TraceConfig::getStoppedCallback() const {
        return tracing_stopped_callback.get();
    }

    TraceConfig &TraceConfig::setStopTracingOnDestruct(bool _stop_tracing) {
        stop_tracing = _stop_tracing;
        return *this;
    }

    bool TraceConfig::getStopTracingOnDestruct() const {
        return stop_tracing;
    }

    TraceConfig &TraceConfig::setCategories(
            const std::vector<std::string> &enabled,
            const std::vector<std::string> &disabled) {
        enabled_categories = enabled;
        disabled_categories = disabled;
        return *this;
    }

    const std::vector<std::string> &TraceConfig::getEnabledCategories() const {
        return enabled_categories;
    }

    const std::vector<std::string> &TraceConfig::getDisabledCategories() const {
        return disabled_categories;
    }

    void TraceConfig::updateFromString(const std::string& config) {
        auto arguments(phosphor::utils::split_string(config, ';'));

        for (const std::string &argument : arguments) {
            auto kv(phosphor::utils::split_string(argument, ':'));

            if (kv.size() != 2) {
                // if split_string does not return a key and a value (2 items)
                throw std::invalid_argument(
                        "TraceConfig::fromString: "
                        "Invalid arguments provided. Arguments must be "
                        "given in as 'key:value;' pairs");
            };

            std::string key(kv[0]);
            std::string value(kv[1]);

            if (key == "buffer-mode") {
                if (value == "fixed") {
                    buffer_factory_container = BufferMode::fixed;
                } else if (value == "ring") {
                    buffer_factory_container = BufferMode::ring;
                } else {
                    throw std::invalid_argument(
                            "TraceConfig::fromString: "
                                    "Invalid buffer mode given");
                }
            } else if (key == "buffer-size") {
                int size;
                try {
                    size = std::stoi(value);
                } catch (std::invalid_argument &) {
                    throw std::invalid_argument(
                            "TraceConfig::fromString: "
                                    "buffer size was not a valid integer");
                } catch (std::out_of_range &) {
                    throw std::invalid_argument(
                            "TraceConfig::fromString: "
                                    "buffer size was too large");
                }

                if (size < 0) {
                    throw std::invalid_argument(
                            "TraceConfig::fromString: "
                                    "buffer size cannot be negative");
                }
                buffer_size = size;
            } else if (key == "save-on-stop") {
                tracing_stopped_callback =
                    std::make_shared<tools::FileStopCallback>(value);
                stop_tracing = true;
            } else if (key == "enabled-categories") {
                enabled_categories = utils::split_string(value, ',');
            } else if (key == "disabled-categories") {
                disabled_categories = utils::split_string(value, ',');
            }
        }
    }

    TraceConfig TraceConfig::fromString(const std::string& config) {
        TraceConfig config_obj(BufferMode::fixed,
                               static_cast<size_t>(1024 * 1024 * 8));
        config_obj.setCategories({"*"}, {});
        config_obj.updateFromString(config);
        return config_obj;
    }

    StringPtr TraceConfig::toString() const {
        std::stringstream result;

        result << "buffer-mode:" << buffer_factory_container.mode << ";";
        result << "buffer-size:" << buffer_size << ";";
        result << "enabled-categories:"
               << utils::join_string(enabled_categories, ',') << ";";
        result << "disabled-categories:"
               << utils::join_string(disabled_categories, ',') << "";

        // Can't easily do the 'save-on-stop' callback

        return make_String(result.str());
    }

    TraceConfig::BufferFactoryContainer::BufferFactoryContainer(BufferMode m)
      : mode(m),
        factory(modeToFactory(m)) {
    }

    TraceConfig::BufferFactoryContainer::BufferFactoryContainer(trace_buffer_factory f)
      : mode(BufferMode::custom),
        factory(f) {
    }

}
