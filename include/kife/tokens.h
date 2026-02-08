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

#ifndef KIFE_TOKENS_H_
#define KIFE_TOKENS_H_

#include "defs.h"

namespace kife
{
    enum token_type_t: uint8_t
    {
        TT_IDENTIFIER,          // Identifier
        TT_NUMERIC,             // Numeric constant
        TT_STRING,              // String literal
        TT_CHARACTER,           // Character

        TT_UNKNOWN,             // Unknown token
        TT_END = TT_UNKNOWN,    // Special marker
    };

} /* namespace kife */

#include <kife/tokens/TokenSet.h>
#include <kife/tokens/TokenSet.hpp>

#endif /* KIFE_TOKENS_H_ */
