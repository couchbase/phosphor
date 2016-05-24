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

#include <sstream>
#include <thread>
#include <vector>

#include "gsl_p/dyn_array.h"

int main(int argc, char* argv[]) {
    std::vector<std::vector<int>> marray;
    marray.push_back(std::vector<int>({1, 2, 3}));
    marray.push_back(std::vector<int>({4, 5, 6}));

    gsl_p::multidimensional_iterator<decltype(marray)::iterator> start(marray.begin());
    gsl_p::multidimensional_iterator<
            decltype(marray)::iterator> finish(marray.end());


    for(auto iter = start; iter != finish; ++iter) {
        std::cout << *iter << std::endl;
    }

//    phosphor::TraceLog::getInstance().start(
//            phosphor::TraceConfig(phosphor::BufferMode::fixed, 1)
//    );
//
//    std::vector<std::thread> threads;
//    for(int i = 0; i < 5; i++) {
//        threads.emplace_back([i]() {
//            phosphor::TraceLog::registerThread();
//            while(phosphor::TraceLog::getInstance().isEnabled()) {
//                TRACE_INSTANT("Child", "Thread #", i, "");
//            }
//            phosphor::TraceLog::deregisterThread();
//        });
//    }
//
//
//    while(phosphor::TraceLog::getInstance().isEnabled()) {
//        TRACE_INSTANT("Main", "Thread", 4, 5);
//    }
//    phosphor::TraceLog::getInstance().stop();
//    auto buffer(phosphor::TraceLog::getInstance().getBuffer());
//
//    for(auto& thread : threads) {
//        thread.join();
//    }
//
//    for (const auto& chunk : buffer->chunks()) {
//        printf("\n\n[NEW CHUNK]\n");
//        for(const auto& event : chunk) {
//            printf("%s\n", event.to_string().c_str());
//        }
//    }


    return 0;
}
