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
        TT_LINE_COMMENT,        // Single-line comment: // ...
        TT_MULTILINE_COMMENT,   // Multi-line comment: /* ... */

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
        TT_QUESTION,            // Question: ?
        TT_DOUBLE_QUESTION,     // Double question: ??
        TT_OPTIONAL_COLON,      // Optional colon: ?:
        TT_OPTIONAL_DOT,        // Optional dot: ?.
        TT_POINTER,             // Pointer access: ->
        TT_ASSIGN,              // Assign: =

        // Mathematics
        TT_PLUS,                // Plus: +
        TT_PLUS_ASSIGN,         // Plus-assign: +=
        TT_MINUS,               // Minus: -
        TT_MINUS_ASSIGN,        // Minus-assign: -=
        TT_INCREMENT,           // Increment: ++
        TT_DECREMENT,           // Decrement: --
        TT_MUL,                 // Mul: *
        TT_MUL_ASSIGN,          // Mul-assign: *=
        TT_DIV,                 // Div: /
        TT_DIV_ASSIGN,          // Div-assign: /=
        TT_MOD,                 // Mod: %
        TT_MOD_ASSIGN,          // Mod-assign: %=

        // Comparison
        TT_LESS,                // Less: <
        TT_LESS_EQ,             // Less or equal: <=
        TT_GREATER,             // Greater: >
        TT_GREATER_EQ,          // Greater or equal: >=
        TT_EQUAL,               // Equal: ==
        TT_NOT_EQUAL,           // Not equal: !=
        TT_THREE_WAY,           // Three way comparison: <=>

        // Logical operations
        TT_NOT,                 // Logical Not: !
        TT_LOG_AND,             // Logical And: &&
        TT_LOG_AND_ASSIGN,      // Logical And-assign: &&=
        TT_LOG_OR,              // Logical Or: ||
        TT_LOG_OR_ASSIGN,       // Logical Or-asign: ||=
        TT_LOG_XOR,             // Logical Xor: ^^
        TT_LOG_XOR_ASSIGN,      // Logical Xor-assign: ^^=

        // Bit operations
        TT_NEG,                 // Binary not: ~
        TT_AND,                 // Binary and: &
        TT_AND_ASSIGN,          // Binary and-assign: &=
        TT_OR,                  // Binary or: |
        TT_OR_ASSIGN,           // Binary or-assign: |=
        TT_XOR,                 // Binary xor: ^
        TT_XOR_ASSIGN,          // Binary xor-assign: ^=
        TT_SHL,                 // Left shift: <<
        TT_SHL_ASSIGN,          // Left shift-assign: <<=
        TT_SHR,                 // Right shift: >>
        TT_SHR_ASSIGN,          // Right shift-assign: >>=
        TT_USHR,                // Unsigned right shift: >>>
        TT_USHR_ASSIGN,         // Unsigned right shift-assign: >>>=
        TT_ROL,                 // Cyclic left shift rotation: <=<
        TT_ROL_ASSIGN,          // Cyclic left shift rotation-assign: <=<=
        TT_ROR,                 // Cyclic right shift rotation: >=>
        TT_ROR_ASSIGN,          // Cyclic right shift rotation-assign: >=>=

        TT_UNEXPECTED,          // Unexpected token
        TT_EOF,                 // Unexpected end of file while reading token

        TT_UNKNOWN,             // Unknown token
        TT_END = TT_UNKNOWN,    // Special marker
    };

    typedef struct token_t
    {
        token_type_t type;      // Decoded token type
        const codepoint_t *data;// Token raw contents

        const char *file;       // File name
        size_t line;            // Line
        size_t column;          // Column
    } token_t;

} /* namespace kife */

#include <kife/tokens/TokenSet.h>
#include <kife/tokens/TokenSet.hpp>

#endif /* KIFE_TOKENS_H_ */
