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
            static constexpr size_t MAX_UNGETCH = 4;

        private:
            typedef struct char_t
            {
                size_t          nLine;                  // Line
                size_t          nColumn;                // Column
                codepoint_t     nCode;                  // Char code point
            } char_t;

            typedef struct token_t
            {
                token_type_t    enType;         // Decoded token type
                codepoint_t    *vBuffer;        // Token raw contents
                size_t          nBufCap;        // Token buffer capacity
                size_t          nBufSize;       // Token buffer size
                size_t          nLine;          // Line
                size_t          nColumn;        // Column

                bool            bUnget;         // Unget flag
            } token_t;

        private:
            char           *vBuffer;                // I/O buffer
            size_t          nBufSize;               // Buffer size
            size_t          nOffset;                // Buffer offset
            size_t          nLine;                  // Line
            size_t          nColumn;                // Column
            size_t          nUngetch;               // Number of elements in the ungetch buffer
            bool            bClose;                 // Close identifier
            FILE           *pFD;                    // Associated file descriptor

            char_t          vUngetch[MAX_UNGETCH];  // Current char
            token_t         sToken;                 // Current token

        private:
            /**
             * Get character from stream. If there is data in the ungetch buffer,
             * then the character is extracted from the ungetch buffer.
             * @return status of operation
             */
            status_t        getch(char_t & ch);

            /**
             * Unget character
             */
            inline status_t ungetch(const char_t & ch);

            /**
             * Put char to token buffer
             * @param c character to put
             * @return status of operation
             */
            status_t        putch(const codepoint_t c);

            /**
             * Put char to token buffer
             * @param c character to put
             * @return status of operation
             */
            inline status_t putch(const char_t & c);

            /**
             * Put buffer of characters
             * @param c buffer of characters
             * @param count number of characters
             * @return status of operation
             */
            status_t        putch(const char_t * c, size_t n);

            /**
             * Update current position depending on the contents of character
             */
            inline void     update_position(const char_t & ch);

            template <typename ... Args>
            inline bool     lookup(char_t & ch, const TokenSet & allowed, Args && ... args);

        private:
            static inline bool      is_blank(const char_t & ch);
            static inline int32_t   parse_hex(const char_t & ch);
            static inline int32_t   parse_dec(const char_t & ch);

        private:
            /**
             * Read single-line comment to the token
             * @return status of operation
             */
            status_t        read_single_line_comment();

            /**
             * Read multi-line comment to the token
             * @return status of operation
             */
            status_t        read_multi_line_comment();

            /**
             * Read single character
             * @return status of operation
             */
            status_t        read_character();

            /**
             * Read string sequence
             * @return status of operation
             */
            status_t        read_string();

            /**
             * Read hexadecimal codepoint
             * @param c codepoint to store value
             * @param digits number of digits to read
             * @return status of operation
             */
            status_t        read_hex_codepoint(codepoint_t & c, size_t digits);

            /**
             * Read variable-length hexadecimal codepoint
             * @param c codepoint to store value
             * @return status of operation
             */
            status_t        read_hex_codepoint(codepoint_t & c);

            /**
             * Read variable-length decimal codepoint
             * @param c codepoint to store value
             * @return status of operation
             */
            status_t        read_dec_codepoint(codepoint_t & c);

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
