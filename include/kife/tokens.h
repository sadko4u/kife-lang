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

        // Comments
        TT_COMMENT,             // Comment: /* ... */ OR // ...

        // Syntax
        TT_LBRACKET,            // Open bracket: (
        TT_RBRACKET,            // Close bracket: )
        TT_LBRACE,              // Open brace: {
        TT_RBRACE,              // Close brace: }
        TT_LQBRACKET,           // Open quad bracket: [
        TT_RQBRACKET,           // Close quad bracket: ]
        TT_COLON,               // Colon: :
        TT_SEMICOLON,           // Semicolon: ;
        TT_COMMA,               // Comma: ,
        TT_DOT,                 // Dot: .

        // Mathematics
        TT_PLUS,                // Plus: +
        TT_MINUS,               // Minus: -
        TT_INCREMENT,           // Increment: ++
        TT_DECREMENT,           // Decrement: --
        TT_MUL,                 // Mul: *
        TT_DIV,                 // Div: /
        TT_MOD,                 // Mod: %

        // Comparison
        TT_LESS,                // Less: <
        TT_LESS_EQ,             // Less or equal: <=
        TT_GREATER,             // Greater: >
        TT_GREATER_EQ,          // Greater or equal: >=
        TT_EQUAL,               // Equal: ==
        TT_NOT_EQUAL,           // Not equal: !=
        TT_THREE_WAY,           // Three way comparison: <=>

        // Logical operations
        TT_NOT,                 // Not: !
        TT_LOG_AND,             // And: &&
        TT_LOG_OR,              // Or: ||
        TT_LOG_XOR,             // Xor: ^^

        // Bit operations
        TT_NEG,                 // Binary not: ~
        TT_AND,                 // Binary and: &
        TT_OR,                  // Binary or: |
        TT_XOR,                 // Binary xor: ^
        TT_SHL,                 // Left shift: <<
        TT_SSHR,                // Signed right shift: >>
        TT_USHR,                // Unsigned right shift: >>>
        TT_ROL,                 // Left shift rotation: <=<
        TT_ROR,                 // Right shift rotation: >=>

        TT_UNKNOWN,             // Unknown token
        TT_END = TT_UNKNOWN,    // Special marker
    };

    typedef struct token_t
    {
        token_type_t type;      // Decoded token type
        const char *data;       // Token raw contents

        const char *file;       // File name
        size_t line;            // Line
        size_t column;          // Column
    } token_t;

} /* namespace kife */

#include <kife/tokens/TokenSet.h>
#include <kife/tokens/TokenSet.hpp>

#endif /* KIFE_TOKENS_H_ */
