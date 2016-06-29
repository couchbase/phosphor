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

#include <phosphor/phosphor.h>

/** \file
 * This file presents a sample implementation of merge sort that has been
 * instrumented with two scoped traces in the merge<T> function and
 * merge_sort<T>.
 */

template <class T>
std::vector<T> merge(const std::vector<T>&& left,
                     const std::vector<T>&& right) {
    TRACE_EVENT("merge_sort", "merge", left.size(), right.size());

    std::vector<T> result;
    auto left_iter = left.begin();
    auto right_iter = right.begin();

    while (left_iter != left.end() && right_iter != right.end()) {
        if (*left_iter >= *right_iter) {
            result.push_back(*left_iter);
            left_iter++;
        } else {
            result.push_back(*right_iter);
            right_iter++;
        }
    }

    if (left_iter != left.end()) {
        result.insert(result.end(), left_iter, left.end());
    } else {
        result.insert(result.end(), right_iter, right.end());
    }

    return result;
}

template <class T>
std::vector<T> merge_sort(const std::vector<T>& input) {
    TRACE_EVENT("merge_sort", "merge_sort", input.size());

    if (input.size() > 1) {
        std::vector<T> left{merge_sort(
            std::vector<T>(input.begin(), input.begin() + (input.size() / 2)))};
        std::vector<T> right{merge_sort(
            std::vector<T>(input.begin() + (input.size() / 2), input.end()))};

        return merge(std::move(left), std::move(right));

    } else {
        return input;
    }
}

int main(int argc, char** argv) {
    std::vector<int> my_list = {1, 5, 3, 67, 8, 3, 36, 546, 77, 32,
                                1, 5, 3, 67, 8, 3, 36, 546, 77, 32,
                                1, 5, 3, 67, 8, 3, 36, 546, 77, 32};

    std::cout << "Presort: ";
    for (int num : my_list) {
        std::cout << num << ", ";
    }
    std::cout << "\n\n";
    std::cout << "Post-sort: ";
    for (int num : merge_sort(my_list)) {
        std::cout << num << ", ";
    }
    std::cout << "\n";
    return 0;
}