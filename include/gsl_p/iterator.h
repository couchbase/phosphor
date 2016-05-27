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
     * @tparam T parent iterator type (e.g. std::vector<std::vector<int>>::iterator)
     */
    template<typename T>
    class multidimensional_iterator {
        using __self = multidimensional_iterator<T>;
        using U = decltype((*((T*)nullptr))->begin());
        using child_value_type = typename std::iterator_traits<U>::value_type;
    public:
        multidimensional_iterator(T parent_, T parent_end_)
            : parent(parent_),
              parent_end(parent_end_),
              child(parent->begin()) {
            seekChildForward(); // Move forward until a valid child is found
        }

        const child_value_type& operator*() const {
            return *child;
        }

        const child_value_type* operator->() const {
            return &(*child);
        }

        __self& operator++() {
            ++child;
            seekChildForward(); // Move forward until a valid child is found
            return *this;
        }

        bool operator==(const __self& other) const {
            // Special case if we're at the end as the child might
            // not be valid
            if(parent == parent_end && parent == other.parent) {
                return true;
            }
            return (parent == other.parent) && (child == other.child);
        }

        bool operator!=(const __self& other) const {
            return !(*this == other);
        }
    protected:
        void seekChildForward() {
            if(parent == parent_end) {
                return; // Already reached the last parent
            }
            while(child == parent->end()) {
                ++parent;
                if(parent == parent_end) {
                    break; // Reached the last parent
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