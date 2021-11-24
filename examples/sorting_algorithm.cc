/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
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
    TRACE_EVENT2("merge_sort", "merge",
                 "left_size", left.size(),
                 "right_size", right.size());

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
    TRACE_EVENT1("merge_sort", "merge_sort", "input_size", input.size());

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