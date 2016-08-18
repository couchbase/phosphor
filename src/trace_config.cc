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

/*
 * TraceLogConfig implementation
 */
    TraceLogConfig::TraceLogConfig()
// Benchmarking suggests that 4x the number of logical
// cores is the sweetspot for the number to share.
            : sentinel_count(std::thread::hardware_concurrency() * 4) {}

    TraceLogConfig &TraceLogConfig::setSentinelCount(unsigned _sentinel_count) {
        sentinel_count = _sentinel_count;
        return *this;
    }

    unsigned TraceLogConfig::getSentinelCount() const {
        return sentinel_count;
    }

    TraceLogConfig &TraceLogConfig::setStartupTrace(
            const TraceConfig &_startup_trace) {
        startup_trace = utils::make_unique<TraceConfig>(_startup_trace);
        return *this;
    }

    TraceLogConfig &TraceLogConfig::clearStartupTrace() {
        startup_trace.release();
        return *this;
    }

    TraceConfig *TraceLogConfig::getStartupTrace() const {
        return startup_trace.get();
    }

    TraceLogConfig &TraceLogConfig::fromEnvironment() {
        const char *sentinel_count_s = std::getenv("PHOSPHOR_SENTINEL_COUNT");
        if (sentinel_count_s && strlen(sentinel_count_s)) {
            int sentinel_count;
            try {
                sentinel_count = std::stoi(sentinel_count_s);
            } catch (std::invalid_argument &) {
                throw std::invalid_argument(
                        "TraceLogConfig::fromEnviroment: "
                        "PHOSPHOR_SENTINEL_COUNT was not a valid integer");
            } catch (std::out_of_range &) {
                throw std::invalid_argument(
                        "TraceLogConfig::fromEnviroment: "
                        "PHOSPHOR_SENTINEL_COUNT was too large");
            }

            if (sentinel_count < 0) {
                throw std::invalid_argument(
                        "TraceLogConfig::fromEnviroment: "
                        "PHOSPHOR_SENTINEL_COUNT cannot be negative");
            }

            this->setSentinelCount(static_cast<unsigned>(sentinel_count));
        }

        const char *startup_config = std::getenv("PHOSPHOR_TRACING_START");
        if (startup_config && strlen(startup_config)) {
            this->setStartupTrace(TraceConfig::fromString(startup_config));
        }

        return *this;
    }

/*
 * TraceConfig implementation
 */

    TraceConfig::TraceConfig(BufferMode _buffer_mode, size_t _buffer_size)
            : buffer_mode(_buffer_mode),
              buffer_size(_buffer_size),
              buffer_factory(modeToFactory(_buffer_mode)),
              enabled_categories({{"*"}}) {}

    TraceConfig::TraceConfig(trace_buffer_factory _buffer_factory,
                             size_t _buffer_size)
            : buffer_mode(BufferMode::custom),
              buffer_size(_buffer_size),
              buffer_factory(_buffer_factory),
              enabled_categories({{"*"}}) {}

    trace_buffer_factory TraceConfig::modeToFactory(BufferMode mode) {
        switch (mode) {
            case BufferMode::fixed:
                return trace_buffer_factory(make_fixed_buffer);
            case BufferMode::ring:
                return trace_buffer_factory(make_ring_buffer);
            case BufferMode::custom:
                throw std::invalid_argument(
                        "phosphor::TraceConfig::modeToFactory: "
                                "Cannot get factory for Custom Mode");
        }
        throw std::invalid_argument(
                "phosphor::TraceConfig::modeToFactory:Invalid buffer mode");
    }

    BufferMode TraceConfig::getBufferMode() const {
        return buffer_mode;
    }

    trace_buffer_factory TraceConfig::getBufferFactory() const {
        return buffer_factory;
    }

    size_t TraceConfig::getBufferSize() const {
        return buffer_size;
    }

    TraceConfig &TraceConfig::setStoppedCallback(
            TracingStoppedCallback _tracing_stopped_callback) {
        tracing_stopped_callback = _tracing_stopped_callback;
        return *this;
    }

    TracingStoppedCallback TraceConfig::getStoppedCallback() const {
        return tracing_stopped_callback;
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

    TraceConfig TraceConfig::fromString(const std::string &config) {
        auto arguments(phosphor::utils::split_string(config, ';'));

        BufferMode mode = BufferMode::fixed;
        int buffer_size = 1024 * 1024 * 8;
        std::string filename = "";
        std::string enabled_categories = "*";
        std::string disabled_categories = "";

        for (const std::string &argument : arguments) {
            auto kv(phosphor::utils::split_string(argument, ':'));

            std::string key(kv[0]);
            std::string value(kv[1]);

            if (key == "buffer-mode") {
                if (value == "fixed") {
                    mode = BufferMode::fixed;
                } else if (value == "ring") {
                    mode = BufferMode::ring;
                } else {
                    throw std::invalid_argument(
                            "TraceConfig::fromString: "
                                    "Invalid buffer mode given");
                }
            } else if (key == "buffer-size") {
                try {
                    buffer_size = std::stoi(value);
                } catch (std::invalid_argument &e) {
                    throw std::invalid_argument(
                            "TraceConfig::fromString: "
                                    "buffer size was not a valid integer");
                } catch (std::out_of_range &e) {
                    throw std::invalid_argument(
                            "TraceConfig::fromString: "
                                    "buffer size was too large");
                }

                if (buffer_size < 0) {
                    throw std::invalid_argument(
                            "TraceConfig::fromString: "
                                    "buffer size cannot be negative");
                }
            } else if (key == "save-on-stop") {
                filename = value;
            } else if (key == "enabled-categories") {
                enabled_categories = value;
            } else if (key == "disabled-categories") {
                disabled_categories = value;
            }
        }

        TraceConfig config_obj(mode, static_cast<size_t>(buffer_size));
        if (filename != "") {
            config_obj.setStoppedCallback(tools::FileStopCallback(filename));
            config_obj.setStopTracingOnDestruct(true);
        }
        config_obj.setCategories(utils::split_string(enabled_categories, ','),
                                 utils::split_string(disabled_categories, ','));
        return config_obj;
    }

    std::string TraceConfig::toString() const {
        std::stringstream result;

        result << "buffer-mode:" << buffer_mode << ";";
        result << "buffer-size:" << buffer_size << ";";
        result << "enabled-categories:"
               << utils::join_string(enabled_categories, ',') << ";";
        result << "disabled-categories:"
               << utils::join_string(disabled_categories, ',') << "";

        // Can't easily do the 'save-on-stop' callback

        return result.str();
    }
}
