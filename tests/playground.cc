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

#include "phosphor/phosphor.h"

#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

#include "gsl_p/dyn_array.h"

int main(int argc, char* argv[]) {
    phosphor::TraceLog::getInstance().start(
            phosphor::TraceConfig(phosphor::BufferMode::fixed, 1000000)
    );

    std::vector<std::thread> threads;
    for(int i = 0; i < 16; i++) {
        threads.emplace_back([i]() {
            //phosphor::TraceLog::registerThread();
            while(phosphor::TraceLog::getInstance().isEnabled()) {
                TRACE_EVENT_START("child", "thread", i, "");
                TRACE_EVENT_START("child", "inner", i, "");
                TRACE_EVENT_START("child", "reallyinn", i, "");
                std::this_thread::yield();
                TRACE_EVENT_END("child", "reallyinn", i, "");
                TRACE_EVENT_END("child", "inner", i, "");
                TRACE_EVENT_END("child", "thread", i, "");
            }
            //phosphor::TraceLog::deregisterThread();
        });
    }


    while(phosphor::TraceLog::getInstance().isEnabled()) {
        TRACE_EVENT_START("main", "thread", 4, 5);
        TRACE_EVENT_END("main", "thread", 4, 5);
    }
    phosphor::TraceLog::getInstance().stop();
    auto buffer(phosphor::TraceLog::getInstance().getBuffer());

    for(auto& thread : threads) {
        thread.join();
    }

    std::fstream fs;
    fs.open("/Users/will/output.json", std::fstream::out | std::fstream::trunc);

    fs << "[";
    bool first = true;
    for (const auto& chunk : buffer->chunks()) {
        //printf("\n\n[NEW CHUNK]\n");
        for(const auto& event : chunk) {
            fs << event.to_json() << ",\n";
            //printf("%s\n", event.to_string().c_str());
        }
    }

    return 0;
}
