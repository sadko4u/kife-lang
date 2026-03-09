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

#ifndef KIFE_DEFS_H_
#define KIFE_DEFS_H_

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#ifdef KIFE_DEBUG
    #define IF_KIFE_DEBUG(...)  __VA_ARGS__
#else
    #define IF_KIFE_DEBUG(...)
#endif /* KIFE_DEBUG */

namespace kife
{
    #if defined(__WORDSIZE) && (__WORDSIZE == 64)
        #define KIFE_ARCH_64BIT
    #elif defined(__SIZE_WIDTH__) && (__SIZE_WIDTH__ == 64)
        #define KIFE_ARCH_64BIT
    #elif defined(__WORDSIZE) && (__WORDSIZE == 32)
        #define KIFE_ARCH_32BIT
    #elif defined(__SIZE_WIDTH__) && (__SIZE_WIDTH__ == 32)
        #define KIFE_ARCH_32BIT
    #else
        #error "Unsupported architecture bitness"
    #endif /* __WORDSIZE, __SIZE_WIDTH__ */

#if defined(KIFE_ARCH_64BIT)
    typedef uint64_t            umword_t;
    typedef int64_t             smword_t;

#elif defined(KIFE_ARCH_32BIT)
    typedef uint32_t            umword_t;
    typedef int32_t             smword_t;
#else
    #error "Unsupported architecture bitness"
#endif

    constexpr size_t UMWORD_BYTES       = sizeof(umword_t);
    constexpr size_t UMWORD_BITS        = sizeof(umword_t) * 8;
    constexpr size_t UMWORD_MIN         = 0;
    constexpr size_t UMWORD_MAX         = ~umword_t(0);

    constexpr size_t SMWORD_BYTES       = sizeof(smword_t);
    constexpr size_t SMWORD_BITS        = sizeof(smword_t) * 8;
    constexpr size_t SMWORD_MIN         = smword_t(UMWORD_MAX >> 1);
    constexpr size_t SMWORD_MAX         = smword_t((UMWORD_MAX >> 1) - 1);

    typedef uint32_t                    codepoint_t;
} /* namespace kife */

#include <kife/defs/variadic.hpp>
#include <kife/defs/utility.hpp>
#include <kife/defs/status.hpp>
#include <kife/defs/finally.hpp>

#endif /* KIFE_TYPES_H_ */
