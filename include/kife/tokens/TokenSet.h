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

#ifndef KIFE_TOKENS_TOKENSET_H_
#define KIFE_TOKENS_TOKENSET_H_

#include <kife/tokens.h>
#include <kife/defs.h>

namespace kife
{
    class TokenSet final
    {
        private:
            static constexpr size_t NUM_WORDS   = (TT_END + UMWORD_BITS - 1) / UMWORD_BITS;
            static constexpr size_t NUM_BYTES   = NUM_WORDS * UMWORD_BYTES;

        private:
            umword_t       vBitMask[NUM_WORDS];

        public:
            inline TokenSet() noexcept;
            inline TokenSet(const TokenSet & src) noexcept;
            inline TokenSet(TokenSet && src) noexcept;
            template <typename ... Args>
            inline TokenSet(const TokenSet & src, Args && ... args) noexcept;
            template <typename ... Args>
            inline TokenSet(Args && ... args) noexcept;

            inline ~TokenSet() noexcept;

        public:
            template <typename First, typename ... Second>
            inline bool contains(First item, Second && ... next) const noexcept;
            inline bool contains(token_type_t token) const noexcept;
            inline bool contains(const TokenSet & set) const noexcept;
            inline TokenSet & clear() noexcept;

            inline TokenSet & assign(const TokenSet & src) noexcept;
            inline TokenSet & assign(TokenSet && src) noexcept;

            inline TokenSet & set(token_type_t token) noexcept;
            TokenSet & set(const TokenSet & src) noexcept;
            template <typename First, typename ... Second>
            inline TokenSet & set(First item, Second && ... next) noexcept;

            inline TokenSet & reset(token_type_t token) noexcept;
            TokenSet & reset(const TokenSet & src) noexcept;
            template <typename First, typename ... Second>
            inline TokenSet & reset(First item, Second && ... next) noexcept;

        public:
            inline bool operator [](token_type_t token) const noexcept;
            inline TokenSet & operator = (const TokenSet & src) noexcept;
            inline TokenSet & operator = (TokenSet && src) noexcept;
    };
} /* namespace kife */

#endif /* KIFE_TOKENS_TOKENSET_H_ */
