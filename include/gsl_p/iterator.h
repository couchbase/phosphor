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

namespace gsl_p {

    /**
     * An iterator for iterating over a container of containers
     *
     * Example: a vector of vectors of int
     *
     *    std::vector<std::vector<int>> vecvec;
     *    vecvec.push_back(std::vector<int>({1, 2}));
     *    vecvec.push_back(std::vector<int>({4, 5}));
     *
     *    gsl_p::multidimensional_iterator<
     *            decltype(vecvec)::iterator> start(vecvec.begin());
     *    gsl_p::multidimensional_iterator<
     *            decltype(vecvec)::iterator> finish(vecvec.end());
     *
     *    for(auto iter = start; iter != finish; ++iter) {
     *        std::cout << *iter << std::endl;
     *    }
     *
     * Output:
     *
     *    1
     *    2
     *    4
     *    5
     *
     * @tparam T parent iterator type (e.g.
     * std::vector<std::vector<int>>::iterator)
     */
    template <typename T>
    class multidimensional_iterator {
        using __self = multidimensional_iterator<T>;
        using U = decltype((*((T*)nullptr))->begin());
        using child_value_type = typename std::iterator_traits<U>::value_type;
        using parent_value_type = typename std::iterator_traits<T>::value_type;

    public:
        multidimensional_iterator(T parent_, T parent_end_)
            : parent(parent_), parent_end(parent_end_), child(parent->begin()) {
            seekChildForward();  // Move forward until a valid child is found
        }

        const child_value_type& operator*() const {
            return *child;
        }

        const child_value_type* operator->() const {
            return &(*child);
        }

        __self& operator++() {
            ++child;
            seekChildForward();  // Move forward until a valid child is found
            return *this;
        }

        bool operator==(const __self& other) const {
            // Special case if we're at the end as the child might
            // not be valid
            if (parent == parent_end && parent == other.parent) {
                return true;
            }
            return (parent == other.parent) && (child == other.child);
        }

        bool operator!=(const __self& other) const {
            return !(*this == other);
        }

        const parent_value_type& getParent() const {
            return *parent;
        }

    protected:
        void seekChildForward() {
            if (parent == parent_end) {
                return;  // Already reached the last parent
            }
            while (child == parent->end()) {
                ++parent;
                if (parent == parent_end) {
                    break;  // Reached the last parent
                }
                child = parent->begin();
            }
        }

    private:
        T parent;
        T parent_end;
        U child;
    };
}