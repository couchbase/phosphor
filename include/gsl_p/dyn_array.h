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
/** \file
 * The following is the experimental dyn_array, as described in the C++ core
 * guidelines and early C++14 specs, produced by the LLVM compiler team. It has
 * been slightly modified to remove dependencies on libc++ and to place it
 * inside of the 'gsl_p' (Guideline support library, phosphor) namespace.
 *
 * dyn_array is *similar* to a std::vector in that the size of the array is
 * determined at runtime. It differs from std::vector significantly from
 * std::vector in that once the dyn_array is constructed, its size is fixed.
 *
 * This has a number of advantages, such as a much simpler implementation. One
 * of the key features is the underlying data is guaranteed not to move once
 * during the lifetime of the dyn_array.
 */

//===-------------------------- dynarray ----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _GSLP_dyn_array
#define _GSLP_dyn_array

/*
    dyn_array synopsis

namespace std { namespace experimental {

template< typename T >
class dyn_array
{
    // types:
    typedef       T                               value_type;
    typedef       T&                              reference;
    typedef const T&                              const_reference;
    typedef       T*                              pointer;
    typedef const T*                              const_pointer;
    typedef       implementation-defined          iterator;
    typedef       implementation-defined          const_iterator;
    typedef reverse_iterator<iterator>            reverse_iterator;
    typedef reverse_iterator<const_iterator>      const_reverse_iterator;
    typedef size_t                                size_type;
    typedef ptrdiff_t                             difference_type;

public:
    // construct/copy/destroy:
    explicit dyn_array(size_type c);
    dyn_array(size_type c, const T& v);
    dyn_array(const dyn_array& d);
    dyn_array(initializer_list<T>);

    template <class Alloc>
      dyn_array(allocator_arg_t, const Alloc& a, size_type c, const Alloc& alloc);
    template <class Alloc>
      dyn_array(allocator_arg_t, const Alloc& a, size_type c, const T& v, const Alloc& alloc);
    template <class Alloc>
      dyn_array(allocator_arg_t, const Alloc& a, const dyn_array& d, const Alloc& alloc);
    template <class Alloc>
      dyn_array(allocator_arg_t, const Alloc& a, initializer_list<T>, const Alloc& alloc);
    dyn_array& operator=(const dyn_array&) = delete;
    ~dyn_array();

    // iterators:
    iterator       begin()        noexcept;
    const_iterator begin()  const noexcept;
    const_iterator cbegin() const noexcept;
    iterator       end()          noexcept;
    const_iterator end()    const noexcept;
    const_iterator cend()   const noexcept;

    reverse_iterator       rbegin()        noexcept;
    const_reverse_iterator rbegin()  const noexcept;
    const_reverse_iterator crbegin() const noexcept;
    reverse_iterator       rend()          noexcept;
    const_reverse_iterator rend()    const noexcept;
    const_reverse_iterator crend()   const noexcept;

    // capacity:
    size_type size()     const noexcept;
    size_type max_size() const noexcept;
    bool      empty()    const noexcept;

    // element access:
    reference       operator[](size_type n);
    const_reference operator[](size_type n) const;

    reference       front();
    const_reference front() const;
    reference       back();
    const_reference back()  const;

    const_reference at(size_type n) const;
    reference       at(size_type n);

    // data access:
    T*       data()       noexcept;
    const T* data() const noexcept;

    // mutating member functions:
    void fill(const T& v);
};

}}  // std::experimental

*/

#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <initializer_list>
#include <new>
#include <algorithm>

namespace gsl_p {

template <class _Tp>
struct dyn_array
{
public:
    // types:
    typedef dyn_array __self;
    typedef _Tp                                   value_type;
    typedef value_type&                           reference;
    typedef const value_type&                     const_reference;
    typedef value_type*                           iterator;
    typedef const value_type*                     const_iterator;
    typedef value_type*                           pointer;
    typedef const value_type*                     const_pointer;
    typedef size_t                                size_type;
    typedef ptrdiff_t                             difference_type;
    typedef std::reverse_iterator<iterator>       reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

private:
    size_t                  __size_;
    value_type *            __base_;
    dyn_array () noexcept :  __size_(0), __base_(nullptr) {}

    static inline value_type* __allocate ( size_t count )
    {
        if (std::numeric_limits<size_t>::max() / sizeof (value_type) <= count )
        {
            throw std::out_of_range("out_of_range");
        }
        return static_cast<value_type *> (::operator new(sizeof(value_type) * count));
    }

    static inline void __deallocate ( value_type* __ptr ) noexcept
    {
        ::operator delete(static_cast<void *> (__ptr));
    }

public:

    explicit dyn_array(size_type __c);
    dyn_array(size_type __c, const value_type& __v);
    dyn_array(const dyn_array& __d);
    dyn_array(std::initializer_list<value_type>);
    dyn_array& operator=(const dyn_array&) = delete;
    ~dyn_array();

    // iterators:
    inline iterator       begin()        noexcept { return iterator(data()); }
    inline const_iterator begin()  const noexcept { return const_iterator(data()); }
    inline const_iterator cbegin() const noexcept { return const_iterator(data()); }
    inline iterator       end()          noexcept { return iterator(data() + __size_); }
    inline const_iterator end()    const noexcept { return const_iterator(data() + __size_); }
    inline const_iterator cend()   const noexcept { return const_iterator(data() + __size_); }

    inline reverse_iterator       rbegin()        noexcept { return reverse_iterator(end()); }
    inline const_reverse_iterator rbegin()  const noexcept { return const_reverse_iterator(end()); }
    inline const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
    inline reverse_iterator       rend()          noexcept { return reverse_iterator(begin()); }
    inline const_reverse_iterator rend()    const noexcept { return const_reverse_iterator(begin()); }
    inline const_reverse_iterator crend()   const noexcept { return const_reverse_iterator(begin()); }

    // capacity:
    inline size_type size()     const noexcept { return __size_; }
    inline size_type max_size() const noexcept { return __size_; }
    inline bool      empty()    const noexcept { return __size_ == 0; }

    // element access:
    inline reference       operator[](size_type __n)       { return data()[__n]; }
    inline const_reference operator[](size_type __n) const { return data()[__n]; }

    inline reference       front()       { return data()[0]; }
    inline const_reference front() const { return data()[0]; }
    inline reference       back()        { return data()[__size_-1]; }
    inline const_reference back()  const { return data()[__size_-1]; }

    inline const_reference at(size_type __n) const;
    inline reference       at(size_type __n);

    // data access:
    inline _Tp*       data()       noexcept { return __base_; }
    inline const _Tp* data() const noexcept { return __base_; }

    // mutating member functions:
    inline  void fill(const value_type& __v) { std::fill_n(begin(), __size_, __v); }
};

template <class _Tp>
inline
dyn_array<_Tp>::dyn_array(size_type __c) : dyn_array ()
{
    __base_ = __allocate (__c);
    value_type *__data = data ();
    for ( __size_ = 0; __size_ < __c; ++__size_, ++__data )
        ::new (__data) value_type;
}

template <class _Tp>
inline
dyn_array<_Tp>::dyn_array(size_type __c, const value_type& __v) : dyn_array ()
{
    __base_ = __allocate (__c);
    value_type *__data = data ();
    for ( __size_ = 0; __size_ < __c; ++__size_, ++__data )
        ::new (__data) value_type (__v);
}

template <class _Tp>
inline
dyn_array<_Tp>::dyn_array(std::initializer_list<value_type> __il) : dyn_array ()
{
    size_t sz = __il.size();
    __base_ = __allocate (sz);
    value_type *__data = data ();
    auto src = __il.begin();
    for ( __size_ = 0; __size_ < sz; ++__size_, ++__data, ++src )
        ::new (__data) value_type (*src);
}

template <class _Tp>
inline
dyn_array<_Tp>::dyn_array(const dyn_array& __d) : dyn_array ()
{
    size_t sz = __d.size();
    __base_ = __allocate (sz);
    value_type *__data = data ();
    auto src = __d.begin();
    for ( __size_ = 0; __size_ < sz; ++__size_, ++__data, ++src )
        ::new (__data) value_type (*src);
}

template <class _Tp>
inline
dyn_array<_Tp>::~dyn_array()
{
    value_type *__data = data () + __size_;
    for ( size_t i = 0; i < __size_; ++i )
        (--__data)->_Tp::~_Tp();
    __deallocate ( __base_ );
}

template <class _Tp>
inline
typename dyn_array<_Tp>::reference
dyn_array<_Tp>::at(size_type __n)
{
    if (__n >= __size_)
    {
        throw std::out_of_range("dyn_array::at");
    }
    return data()[__n];
}

template <class _Tp>
inline
typename dyn_array<_Tp>::const_reference
dyn_array<_Tp>::at(size_type __n) const
{
    if (__n >= __size_)
    {
        throw std::out_of_range("dyn_array::at");
    }
    return data()[__n];
}

}

#endif  // _GSLP_dyn_array
