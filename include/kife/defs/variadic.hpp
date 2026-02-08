/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of kife-lang
 * Created on: 8 февр. 2026 г.
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

#ifndef KIFE_TYPES_HPP_
#define KIFE_TYPES_HPP_

namespace kife
{
    // kife::remove_reference
    template <class T>
    struct remove_reference
    {
        typedef T type;
    };

    template <class T>
    struct remove_reference<T&>
    {
        typedef T type;
    };

    template <class T>
    struct remove_reference<T&&>
    {
        typedef T type;
    };

    // kife::remove_const
    template<class T>
    struct remove_const
    {
        typedef T type;
    };

    template<class T>
    struct remove_const<const T>
    {
        typedef T type;
    };

    template <class T>
    struct unconst_ptr_type
    {
        using type = typename remove_const<T>::type;
        static inline type * cast(type * value)         { return static_cast<type *>(value); }
        static inline type * cast(const type * value)   { return const_cast<type *>(value);  }
    };

    // kife::unconst_ptr
    template <typename T>
    typename unconst_ptr_type<T>::type * unconst_ptr(T * t) noexcept
    {
        return unconst_ptr_type<T>::cast(t);
    }

    // kife::move
    template <typename T>
    typename remove_reference<T>::type && move(T && t) noexcept
    {
        return static_cast<typename remove_reference<T>::type &&>(t);
    }

    // kife::forward
    template<typename T>
    constexpr T && forward(typename remove_reference<T>::type && args) noexcept
    {
        return static_cast<T &&>(args);
    }

    template<typename T>
    constexpr T && forward(typename remove_reference<T>::type & args) noexcept
    {
        return static_cast<T &&>(args);
    }
} /* namespace kife */


#endif /* KIFE_TYPES_HPP_ */
