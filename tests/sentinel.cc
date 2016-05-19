#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <cstdio>

#include "sentinel.h"

Sentinel s;
int j;

void worker(int n) {

    for(int i(0); i < 1000; i++) {
        if(!s.acquire()) {
            fprintf(stdout, "Worker<%d> flipped[%d]\n", n, i);
            return;
        }
        j++;
        //fprintf(stdout, "Worker<%d> loop[%d]\n", n, i);
        s.release();
        j--;

    }
    fprintf(stderr, "Worker<%d> finished\n", n);
}

void closer() {
    std::this_thread::sleep_for(std::chrono::nanoseconds(10000000));
    s.close();
    fprintf(stderr, "%s", "Closed sentinel\n");
}

int main(int argc, char* argv[]) {

    std::vector<std::thread> ts;
    for(int i(0); i < 70; i++) {
        ts.emplace_back(worker, i);
    }
    ts.emplace_back(closer);
    for(auto& t : ts) {
        t.join();
    }

    std::cerr << j << std::endl;
    return 0;
}