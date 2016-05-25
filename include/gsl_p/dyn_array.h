/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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

/** LICENSE.TXT: **/
//==============================================================================
//libc++ License
//==============================================================================
//
//The libc++ library is dual licensed under both the University of Illinois
//"BSD-Like" license and the MIT license.  As a user of this code you may choose
//to use it under either license.  As a contributor, you agree to allow your code
//to be used under both.
//
//Full text of the relevant licenses is included below.
//
//==============================================================================
//
//University of Illinois/NCSA
//        Open Source License
//
//Copyright (c) 2009-2016 by the contributors listed in CREDITS.TXT
//
//        All rights reserved.
//
//Developed by:
//
//LLVM Team
//
//University of Illinois at Urbana-Champaign
//
//        http://llvm.org
//
//Permission is hereby granted, free of charge, to any person obtaining a copy of
//this software and associated documentation files (the "Software"), to deal with
//        the Software without restriction, including without limitation the rights to
//use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
//of the Software, and to permit persons to whom the Software is furnished to do
//so, subject to the following conditions:
//
//* Redistributions of source code must retain the above copyright notice,
//this list of conditions and the following disclaimers.
//
//* Redistributions in binary form must reproduce the above copyright notice,
//this list of conditions and the following disclaimers in the
//documentation and/or other materials provided with the distribution.
//
//* Neither the names of the LLVM Team, University of Illinois at
//Urbana-Champaign, nor the names of its contributors may be used to
//endorse or promote products derived from this Software without specific
//        prior written permission.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
//        FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
//        CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
//SOFTWARE.
//
//==============================================================================
//
//Copyright (c) 2009-2014 by the contributors listed in CREDITS.TXT
//
//        Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//        to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//        copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//        The above copyright notice and this permission notice shall be included in
//all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//        AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//        THE SOFTWARE.

/** CREDITS.TXT **/
//This file is a partial list of people who have contributed to the LLVM/libc++
//project.  If you have contributed a patch or made some other contribution to
//        LLVM/libc++, please submit a patch to this file to add yourself, and it will be
//done!
//
//The list is sorted by surname and formatted to allow easy grepping and
//        beautification by scripts.  The fields are: name (N), email (E), web-address
//(W), PGP key ID and fingerprint (P), description (D), and snail-mail address
//        (S).
//
//N: Saleem Abdulrasool
//        E: compnerd@compnerd.org
//        D: Minor patches and Linux fixes.
//
//N: Dan Albert
//        E: danalbert@google.com
//        D: Android support and test runner improvements.
//
//N: Dimitry Andric
//        E: dimitry@andric.com
//        D: Visibility fixes, minor FreeBSD portability patches.
//
//N: Holger Arnold
//        E: holgerar@gmail.com
//        D: Minor fix.
//
//N: Ruben Van Boxem
//E: vanboxem dot ruben at gmail dot com
//D: Initial Windows patches.
//
//N: David Chisnall
//        E: theraven at theravensnest dot org
//        D: FreeBSD and Solaris ports, libcxxrt support, some atomics work.
//
//N: Marshall Clow
//        E: mclow.lists@gmail.com
//        E: marshall@idio.com
//        D: C++14 support, patches and bug fixes.
//
//N: Eric Fiselier
//        E: eric@efcs.ca
//        D: LFTS support, patches and bug fixes.
//
//N: Bill Fisher
//        E: william.w.fisher@gmail.com
//        D: Regex bug fixes.
//
//N: Matthew Dempsky
//        E: matthew@dempsky.org
//        D: Minor patches and bug fixes.
//
//N: Google Inc.
//D: Copyright owner and contributor of the CityHash algorithm
//
//N: Howard Hinnant
//        E: hhinnant@apple.com
//        D: Architect and primary author of libc++
//
//N: Hyeon-bin Jeong
//E: tuhertz@gmail.com
//        D: Minor patches and bug fixes.
//
//N: Argyrios Kyrtzidis
//        E: kyrtzidis@apple.com
//        D: Bug fixes.
//
//N: Bruce Mitchener, Jr.
//E: bruce.mitchener@gmail.com
//        D: Emscripten-related changes.
//
//N: Michel Morin
//        E: mimomorin@gmail.com
//        D: Minor patches to is_convertible.
//
//N: Andrew Morrow
//        E: andrew.c.morrow@gmail.com
//        D: Minor patches and Linux fixes.
//
//N: Arvid Picciani
//        E: aep at exys dot org
//        D: Minor patches and musl port.
//
//N: Bjorn Reese
//        E: breese@users.sourceforge.net
//        D: Initial regex prototype
//
//        N: Nico Rieck
//E: nico.rieck@gmail.com
//        D: Windows fixes
//
//N: Jon Roelofs
//        E: jonathan@codesourcery.com
//        D: Remote testing, Newlib port, baremetal/single-threaded support.
//
//N: Jonathan Sauer
//        D: Minor patches, mostly related to constexpr
//
//N: Craig Silverstein
//        E: csilvers@google.com
//        D: Implemented Cityhash as the string hash function on 64-bit machines
//
//N: Richard Smith
//        D: Minor patches.
//
//N: Joerg Sonnenberger
//        E: joerg@NetBSD.org
//        D: NetBSD port.
//
//N: Stephan Tolksdorf
//        E: st@quanttec.com
//        D: Minor <atomic> fix
//
//N: Michael van der Westhuizen
//        E: r1mikey at gmail dot com
//
//        N: Larisse Voufo
//D: Minor patches.
//
//N: Klaas de Vries
//E: klaas at klaasgaaf dot nl
//D: Minor bug fix.
//
//N: Zhang Xiongpang
//        E: zhangxiongpang@gmail.com
//        D: Minor patches and bug fixes.
//
//N: Xing Xue
//        E: xingxue@ca.ibm.com
//        D: AIX port
//
//N: Zhihao Yuan
//        E: lichray@gmail.com
//        D: Standard compatibility fixes.
//
//N: Jeffrey Yasskin
//        E: jyasskin@gmail.com
//        E: jyasskin@google.com
//        D: Linux fixes.

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
