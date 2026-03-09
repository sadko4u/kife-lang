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

#ifndef KIFE_TOKENS_TOKENSET_HPP_
#define KIFE_TOKENS_TOKENSET_HPP_

#include <kife/tokens/TokenSet.h>

#include <string.h>

namespace kife
{
    // Constructors
    inline TokenSet::TokenSet() noexcept
    {
        bzero(vBitMask, NUM_BYTES);
    }

    inline TokenSet::TokenSet(const TokenSet & src) noexcept
    {
        memcpy(vBitMask, src.vBitMask, NUM_BYTES);
    }

    inline TokenSet::TokenSet(TokenSet && src) noexcept
    {
        memcpy(vBitMask, src.vBitMask, NUM_BYTES);
        IF_KIFE_DEBUG(bzero(src.vBitMask, NUM_BYTES));
    }

    template <typename ... Args>
    inline TokenSet::TokenSet(const TokenSet & src, Args && ... args) noexcept
    {
        memcpy(vBitMask, src.vBitMask, NUM_BYTES);
        set(kife::forward<Args>(args)...);
    }

    template <typename ... Args>
    inline TokenSet::TokenSet(Args && ... args) noexcept
    {
        bzero(vBitMask, NUM_BYTES);
        set(kife::forward<Args>(args)...);
    }

    // Destructor
    inline TokenSet::~TokenSet() noexcept
    {
        IF_KIFE_DEBUG(bzero(vBitMask, NUM_BYTES));
    }

    // Set operations
    inline bool TokenSet::contains(token_type_t token) const noexcept
    {
        const size_t word = token / UMWORD_BITS;
        return (word < NUM_WORDS) ? vBitMask[word] & (umword_t(1) << (token % UMWORD_BITS)) : false;
    }

    inline bool TokenSet::contains(const TokenSet & src) const noexcept
    {
        for (size_t i=0; i<NUM_WORDS; ++i)
            if (vBitMask[i] & src.vBitMask[i])
                return true;
        return false;
    }

    template <typename First, typename ... Second>
    inline bool TokenSet::contains(First item, Second && ... next) const noexcept
    {
        if (contains(item))
            return true;
        return contains(kife::forward<Second>(next)...);
    }

    inline TokenSet & TokenSet::clear() noexcept
    {
        bzero(vBitMask, NUM_BYTES);
        return *this;
    }

    inline TokenSet & TokenSet::assign(const TokenSet & src) noexcept
    {
        memcpy(vBitMask, src.vBitMask, NUM_BYTES);
        return *this;
    }

    inline TokenSet & TokenSet::assign(TokenSet && src) noexcept
    {
        memcpy(vBitMask, src.vBitMask, NUM_BYTES);
        IF_KIFE_DEBUG(bzero(src.vBitMask, NUM_BYTES));
        return *this;
    }

    inline TokenSet & TokenSet::set(token_type_t token) noexcept
    {
        const size_t word   = token / UMWORD_BITS;
        if (word < NUM_WORDS)
            vBitMask[word]     |= (umword_t(1) << (token % UMWORD_BITS));
        return *this;
    }

    template <typename First, typename ... Second>
    inline TokenSet & TokenSet::set(First item, Second && ... next) noexcept
    {
        return set(item).set(kife::forward<Second>(next)...);
    }

    inline TokenSet & TokenSet::reset(token_type_t token) noexcept
    {
        const size_t word  = token / UMWORD_BITS;
        if (word < NUM_WORDS)
            vBitMask[word]     &= ~(umword_t(1) << (token % UMWORD_BITS));
        return *this;
    }

    template <typename First, typename ... Second>
    inline TokenSet & TokenSet::reset(First item, Second && ... next) noexcept
    {
        return reset(item).reset(kife::forward<Second>(next)...);
    }

    // Operators
    inline bool TokenSet::operator [](token_type_t token) const noexcept
    {
        return contains(token);
    }

    inline TokenSet & TokenSet::operator = (const TokenSet & src) noexcept
    {
        return assign(src);
    }

    inline TokenSet & TokenSet::operator = (TokenSet && src) noexcept
    {
        return assign(kife::move(src));
    }

} /* namespace kife */

#endif /* KIFE_TOKENS_TOKENSET_HPP_ */
