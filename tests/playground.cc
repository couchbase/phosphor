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
#include <cassert>

#include "gsl_p/dyn_array.h"

class node {
public:

    node()
        : next(0) {
    }
    std::atomic<node*> next;

    int num;
    std::atomic<int> numat;
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

class mpmc_queue {
public:

    mpmc_queue(node* nodes, size_t len)
       : head(&nodes[0]), tail(&nodes[0]) {
        for(int i = 1; i < len; ++i) enqueue(&nodes[i]);
        count = len;
    }

    void enqueue(node* n) {
        assert(n != nullptr);
        n->next = nullptr;
        while(true) {
            node* tail_expected = tail.load();
            node* next_expected = tail_expected->next.load();
            if(next_expected == nullptr) {
                if(tail_expected->next.compare_exchange_weak(next_expected, n)) {
                    tail.compare_exchange_strong(tail_expected, n);
                    count++;
                    return;
                }
            } else {
                tail.compare_exchange_weak(tail_expected, next_expected);
            }
        }
    }

    node* dequeue() {
        while(true) {
            node* head_expected = head.load();
            node* tail_expected = tail.load();
            node* next_expected = head_expected->next.load();

            if(head_expected == tail_expected) {
                if(next_expected == nullptr) {
                    return nullptr;
                } else {
                    tail.compare_exchange_weak(tail_expected, next_expected);
                }
            } else if(next_expected) {

                if(head.compare_exchange_strong(head_expected, next_expected)) {
                    assert(next_expected != nullptr);
                    //head_expected->next = nullptr;
                    count--;
                    return head_expected;
                }
            }

        }
    }

protected:
    std::atomic<node*> head;
    std::atomic<node*> tail;
    std::atomic<size_t> count;
};

int main(int argc, char* argv[]) {

    mpmc_queue queue{new node, 1};
    for(int i = 0; i < 4; ++i) {
        queue.enqueue(new node);
    }


    std::vector<std::thread> threads;
    for(int i = 0; i < 2; ++i) {
        threads.emplace_back([&queue]() {
            for(int j = 0; j < 100000; ++j) {
                node *n{queue.dequeue()};
                assert(n != nullptr);
                delete n;
                n = new node;
                n->num = 0;
                n->numat = 0;
                n->next = nullptr;
                queue.enqueue(n);
            }
        });
    }

    for(auto& thread : threads) {
        thread.join();
    }

    return 0;
}
