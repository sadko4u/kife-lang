/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of kife-lang
 * Created on: 14 февр. 2026 г.
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

#ifndef KIFE_TOKENS_TOKENIZER_H_
#define KIFE_TOKENS_TOKENIZER_H_

#include <kife/defs.h>
#include <kife/tokens.h>
#include <kife/tokens/TokenSet.h>

#include <stdio.h>

namespace kife
{
    class Tokenizer final
    {
        public:
            static constexpr size_t BUFFER_SIZE = 0x1000;

        private:
            typedef struct char_t
            {
                uint8_t         nCode;          // Current code
                bool            bUnget;         // Unget flag
            } char_t;

            typedef struct token_t
            {
                token_type_t    enType;         // Decoded token type
                char           *vBuffer;        // Token raw contents
                size_t          nBufCap;        // Token buffer capacity
                size_t          nBufSize;       // Token buffer size
                size_t          nLine;          // Line
                size_t          nColumn;        // Column

                bool            bUnget;         // Unget flag
            } token_t;

        private:
            char           *vBuffer;            // I/O buffer
            size_t          nBufSize;           // Buffer size
            size_t          nOffset;            // Buffer offset
            FILE           *pFD;                // Associated file descriptor
            bool            bClose;             // Close identifier

            char_t          sChar;              // Current char
            token_t         sToken;             // Current token

        private:
            /**
             * Get character from stream
             * @return character code
             */
            status_t        getch();

            /**
             * Unget last character from stream
             */
            inline void     ungetch();

            /**
             * Put char to token buffer
             * @param c
             * @return
             */
            status_t        putch(char c);

        public:
            Tokenizer();
            Tokenizer(const Tokenizer &) = delete;
            Tokenizer(Tokenizer &&);
            ~Tokenizer();

            Tokenizer & operator = (const Tokenizer &) = delete;
            Tokenizer & operator = (Tokenizer &&);

        public:
            status_t open(const char *path);
            status_t wrap(FILE *fd, bool close);
            status_t wrap(const void *buf, size_t count, bool free);
            status_t close();

        public:
            /**
             * Get new token. Token points to internal buffers of tokenizer and is valid until next get() call.
             * @param tok token to get
             * @param allowed list of allowed tokens
             * @return status of operation
             */
            status_t        get(kife::token_t & tok, const TokenSet & allowed);

            /**
             * Unget current token
             */
            void            unget();
    };
} /* namespace kife */


#endif /* KIFE_TOKENS_TOKENIZER_H_ */
