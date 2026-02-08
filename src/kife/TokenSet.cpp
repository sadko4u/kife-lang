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

#include <kife/tokens.h>

namespace kife
{
    TokenSet & TokenSet::set(const TokenSet & src) noexcept
    {
        for (size_t i=0; i<NUM_WORDS; ++i)
            vBitMask[i] |= src.vBitMask[i];
        return *this;
    }

    TokenSet & TokenSet::reset(const TokenSet & src) noexcept
    {
        for (size_t i=0; i<NUM_WORDS; ++i)
            vBitMask[i] &= ~src.vBitMask[i];
        return *this;
    }

} /* namespace kife */


