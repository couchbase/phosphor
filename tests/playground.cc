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

#ifdef _WIN32
// Windows doesn't have setenv so emulate it

// StackOverflow @bill-weinman - http://stackoverflow.com/a/23616164/5467841
int setenv(const char *name, const char *value, int overwrite)
{
    int errcode = 0;
    if(!overwrite) {
        size_t envsize = 0;
        errcode = getenv_s(&envsize, NULL, 0, name);
        if(errcode || envsize) return errcode;
    }
    return _putenv_s(name, value);
}
#endif

int main(int argc, char* argv[]) {
    setenv("PHOSPHOR_TRACING_START", "buffer-mode:fixed,buffer-size:10241024", 1);

//    phosphor::TraceLog::getInstance().start(
//            phosphor::TraceConfig(phosphor::BufferMode::fixed, 1000000)
//    );

    std::vector<std::thread> threads;
    for(int i = 0; i < 16; i++) {
        threads.emplace_back([i]() {
            //phosphor::TraceLog::registerThread();
            while(phosphor::TraceLog::getInstance().isEnabled()) {
                TRACE_EVENT("child", "thread", 4, 5);
            }
            //phosphor::TraceLog::deregisterThread();
        });
    }


    while(phosphor::TraceLog::getInstance().isEnabled()) {
        TRACE_EVENT("main", "thread", 4, 5);
    }
    phosphor::TraceLog::getInstance().stop();
    auto buffer(phosphor::TraceLog::getInstance().getBuffer());

    for(auto& thread : threads) {
        thread.join();
    }

    std::fstream fs;
    fs.open("/Users/will/output.json", std::fstream::out | std::fstream::trunc);

    fs << "[";
    for (const auto& chunk : buffer->chunks()) {
        printf("\n\n[NEW CHUNK]\n");
        for(const auto& event : chunk) {
            fs << event.to_json() << ",\n";
            printf("%s\n", event.to_string().c_str());
        }
    }

    return 0;
}
