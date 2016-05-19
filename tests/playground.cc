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

#include "etracer.h"

#include <thread>
#include <vector>

int main(int argc, char* argv[]) {
    TraceLog::getInstance().start(
        TraceConfig(BufferMode::fixed, 1000)
    );

    std::vector<std::thread> threads;
    for(int i = 0; i < 5; i++) {
        threads.emplace_back([i]() {
            TraceLog::registerThread();
            while(TraceLog::getInstance().isEnabled()) {
                TRACE_INSTANT("Child", "Thread #", i, "");
            }
            TraceLog::deregisterThread();
        });
    }


    while(TraceLog::getInstance().isEnabled()) {
        TRACE_INSTANT("Main", "Thread", 4, 5);
    }
    TraceLog::getInstance().stop();
    auto buffer(TraceLog::getInstance().getBuffer());

    for(auto& thread : threads) {
        thread.join();
    }

//    for (const auto& event : *buffer) {
//        std::cout << event << '\n';
//    }



    return 0;
}
