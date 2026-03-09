/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of kife-lang
 * Created on: 15 февр. 2026 г.
 *
 * kife-lang is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * kife-lang is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with kife-lang. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KIFE_DEFS_UTILITY_HPP_
#define KIFE_DEFS_UTILITY_HPP_

#include <kife/defs/variadic.hpp>

namespace kife
{

    template <typename T, typename U = T>
    inline void swap(T & a, U & b)
    {
        T tmp = kife::move(a);
        a = kife::move(b);
        b = kife::move(tmp);
    }

    template<typename T, typename U = T>
    inline T exchange(T & obj, U && new_value)
    {
        T old = kife::move(obj);
        obj = kife::forward<U>(new_value);
        return old;
    }

    template <typename T>
    inline T * release_ptr(T * & ptr)
    {
        T * const old = ptr;
        ptr = nullptr;
        return old;
    }

    template <class A, class B>
    constexpr inline A max(A a, B b)
    {
        return (a > b) ? a : b;
    }

    template <class A, class B, class C>
    constexpr inline A max(A a, B b, C c)
    {
        return ((a > b) && (a > c)) ? a :
                (b > c) ? b : c;
    }

    template <class A, class B, class C, class D>
    constexpr inline A max(A a, B b, C c, D d)
    {
        return lsp_max(lsp_max(a, b), lsp_max(c, d));
    }

    template <class A, class B>
    constexpr inline A min(A a, B b)
    {
        return (a < b) ? a : b;
    }

    template <class A, class B, class C>
    constexpr inline A min(A a, B b, C c)
    {
        return ((a < b) && (a < c)) ? a :
            (b < c) ? b : c;
    }

    template <class A, class B, class C, class D>
    constexpr inline A min(A a, B b, C c, D d)
    {
        return lsp_min(lsp_min(a, b), lsp_min(c, d));
    }

} /* namespace kife */


#endif /* KIFE_DEFS_UTILITY_HPP_ */
