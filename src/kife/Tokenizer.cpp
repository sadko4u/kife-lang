/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of kife-lang
 * Created on: 15 февр. 2026 г.
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

#include <kife/tokens/Tokenizer.h>

#include <stdlib.h>
#include <string.h>

namespace kife
{
    Tokenizer::Tokenizer()
    {
        vBuffer         = nullptr;
        nBufSize        = 0;
        nOffset         = 0;
        nLine           = 1;
        nColumn         = 1;
        nUngetch        = 0;
        bClose          = false;
        pFD             = NULL;

        for (size_t i=0; i<MAX_UNGETCH; ++i)
        {
            char_t * const ch = &vUngetch[i];
            ch->nLine       = 0;
            ch->nColumn     = 0;
            ch->nCode       = 0;
        }

        sToken.enType   = TT_UNKNOWN;
        sToken.vBuffer  = nullptr;
        sToken.nBufCap  = 0;
        sToken.nBufSize = 0;
        sToken.nLine    = 1;
        sToken.nColumn  = 1;
        sToken.bUnget   = false;
    }

    Tokenizer::Tokenizer(Tokenizer && src)
    {
        vBuffer         = release_ptr(src.vBuffer);
        nBufSize        = kife::exchange(src.nBufSize, 0);
        nOffset         = kife::exchange(src.nOffset, 0);
        nLine           = kife::exchange(src.nLine, 1);
        nColumn         = kife::exchange(src.nColumn, 1);
        nUngetch        = kife::exchange(src.nUngetch, 0);
        bClose          = src.bClose;
        pFD             = release_ptr(src.pFD);

        for (size_t i=0; i<MAX_UNGETCH; ++i)
            vUngetch[i]     = kife::move(src.vUngetch[i]);

        sToken.enType   = src.sToken.enType;
        sToken.vBuffer  = release_ptr(src.sToken.vBuffer);
        sToken.nBufCap  = kife::exchange(src.sToken.nBufCap, 0);
        sToken.nBufSize = kife::exchange(src.sToken.nBufSize, 0);
        sToken.nLine    = 1;
        sToken.nColumn  = 1;
        sToken.bUnget   = false;
    }

    Tokenizer::~Tokenizer()
    {
        close();
    }

    Tokenizer & Tokenizer::operator = (Tokenizer && src)
    {
        vBuffer         = release_ptr(src.vBuffer);
        nBufSize        = kife::exchange(src.nBufSize, 0);
        nOffset         = kife::exchange(src.nOffset, 0);
        nLine           = kife::exchange(src.nLine, 1);
        nColumn         = kife::exchange(src.nColumn, 1);
        nUngetch        = kife::exchange(src.nUngetch, 0);
        pFD             = release_ptr(src.pFD);
        bClose          = src.bClose;

        for (size_t i=0; i<MAX_UNGETCH; ++i)
            vUngetch[i]     = kife::move(src.vUngetch[i]);

        sToken.enType   = src.sToken.enType;
        sToken.vBuffer  = release_ptr(src.sToken.vBuffer);
        sToken.nBufCap  = kife::exchange(src.sToken.nBufCap, 0);
        sToken.nBufSize = kife::exchange(src.sToken.nBufSize, 0);
        sToken.nLine    = 1;
        sToken.nColumn  = 1;
        sToken.bUnget   = false;

        return *this;
    }

    status_t Tokenizer::getch(char_t & ch)
    {
        // Char has been unget?
        if (nUngetch > 0)
        {
            ch              = vUngetch[--nUngetch];
            return STATUS_OK;
        }

        // Have enough data in the buffer?
        if (nOffset < nBufSize)
        {
            ch.nLine        = nLine;
            ch.nColumn      = nColumn;
            ch.nCode        = vBuffer[nOffset++];
            update_position(ch);
        }

        // Can fill the buffer ?
        if (pFD == NULL)
            return STATUS_EOF;

        nBufSize = fread(vBuffer, BUFFER_SIZE, 1, pFD);
        if (nBufSize > 0)
        {
            nOffset         = 1;
            ch.nLine        = nLine;
            ch.nColumn      = nColumn;
            ch.nCode        = vBuffer[0];
            update_position(ch);
        }

        return (feof(pFD)) ? STATUS_EOF : STATUS_IO_ERROR;
    }

    void Tokenizer::update_position(const char_t & ch)
    {
        switch (ch.nCode)
        {
            case '\n':
                ++nLine;
                nColumn = 1;
                break;

            default:
                ++nColumn;
                break;
        }
    }

    inline status_t Tokenizer::ungetch(const char_t & ch)
    {
        if (nUngetch >= MAX_UNGETCH)
            return STATUS_OVERFLOW;

        vUngetch[nUngetch++]    = ch;
        return STATUS_OK;
    }

    status_t Tokenizer::open(const char *path)
    {
        if (!path)
            return STATUS_BAD_ARGUMENTS;
        if ((vBuffer) || (pFD))
            return STATUS_BAD_STATE;

        // Open file
        FILE *fd = fopen(path, "r");
        if (!fd)
            return STATUS_IO_ERROR;
        finally {
            if (fd)
                fclose(fd);
        };

        status_t res = wrap(fd, true);
        if (res == STATUS_OK)
            fd      = NULL;

        return res;
    }

    status_t Tokenizer::wrap(FILE *fd, bool close)
    {
        if (!fd)
            return STATUS_BAD_ARGUMENTS;

        // Allocate I/O buffer
        char *iobuf         = static_cast<char *>(malloc(BUFFER_SIZE));
        if (!iobuf)
            return STATUS_NO_MEM;
        finally { free(iobuf);      };

        // Allocate token buffer
        codepoint_t *tokbuf     = static_cast<codepoint_t *>(malloc(BUFFER_SIZE * sizeof(codepoint_t)));
        if (!tokbuf)
            return STATUS_NO_MEM;
        finally { free(tokbuf);     };

        // Commit state
        vBuffer         = release_ptr(iobuf);
        nBufSize        = 0;
        nOffset         = 0;
        nLine           = 1;
        nColumn         = 1;
        nUngetch        = 0;
        bClose          = close;
        pFD             = fd;

        for (size_t i=0; i<MAX_UNGETCH; ++i)
        {
            char_t * const ch = &vUngetch[i];
            ch->nLine       = 0;
            ch->nColumn     = 0;
            ch->nCode       = 0;
        }

        sToken.enType   = TT_UNKNOWN;
        sToken.vBuffer  = release_ptr(tokbuf);
        sToken.nBufCap  = BUFFER_SIZE;
        sToken.nBufSize = 0;
        sToken.nLine    = 1;
        sToken.nColumn  = 1;
        sToken.bUnget   = false;

        return STATUS_OK;
    }

    status_t Tokenizer::wrap(const void *buf, size_t count, bool free)
    {
        if (!buf)
            return STATUS_BAD_ARGUMENTS;

        // Allocate token buffer
        codepoint_t *tokbuf     = static_cast<codepoint_t *>(malloc(BUFFER_SIZE * sizeof(codepoint_t)));
        if (!tokbuf)
            return STATUS_NO_MEM;
        finally { ::free(tokbuf);   };

        // Commit state
        vBuffer         = const_cast<char *>(static_cast<const char *>(buf));
        nBufSize        = count;
        nOffset         = 0;
        nLine           = 1;
        nColumn         = 1;
        nUngetch        = 0;
        bClose          = free;
        pFD             = nullptr;

        for (size_t i=0; i<MAX_UNGETCH; ++i)
        {
            char_t * const ch = &vUngetch[i];
            ch->nLine       = 0;
            ch->nColumn     = 0;
            ch->nCode       = 0;
        }

        sToken.enType   = TT_UNKNOWN;
        sToken.vBuffer  = release_ptr(tokbuf);
        sToken.nBufCap  = BUFFER_SIZE;
        sToken.nBufSize = 0;
        sToken.nLine    = 1;
        sToken.nColumn  = 1;
        sToken.bUnget   = false;

        return STATUS_OK;
    }

    status_t Tokenizer::close()
    {
        if (pFD)
        {
            if (bClose)
                fclose(pFD);
            pFD = nullptr;

            free(vBuffer);
        }
        else
        {
            if (bClose)
                free(vBuffer);
        }

        free(sToken.vBuffer);

        bClose          = false;
        vBuffer         = nullptr;
        sToken.vBuffer  = nullptr;

        return STATUS_OK;
    }

    status_t Tokenizer::putch(codepoint_t c)
    {
        // Reallocate buffer if needed
        if (sToken.nBufSize >= sToken.nBufCap)
        {
            const size_t new_cap    = kife::max(sToken.nBufCap + (sToken.nBufCap << 1), 32u);
            codepoint_t * const ptr = static_cast<codepoint_t *>(realloc(sToken.vBuffer, sizeof(codepoint_t) * new_cap));
            if (!ptr)
                return STATUS_NO_MEM;

            sToken.vBuffer          = ptr;
            sToken.nBufCap          = new_cap;
        }

        sToken.vBuffer[sToken.nBufSize++]   = c;
        return STATUS_OK;
    }

    inline status_t Tokenizer::putch(const char_t & c)
    {
        return putch(c.nCode);
    }

    status_t Tokenizer::putch(const char_t * c, size_t n)
    {
        for (size_t i=0; i<n; ++i)
        {
            status_t res = putch(c[i]);
            if (res != STATUS_OK)
                return res;
        }
        return STATUS_OK;
    }

    inline bool Tokenizer::is_blank(const char_t & ch)
    {
        switch (ch.nCode)
        {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
            case '\v':
                return true;
            default:
                break;
        }
        return false;
    }

    inline int32_t Tokenizer::parse_hex(const char_t & ch)
    {
        const codepoint_t cp = ch.nCode;
        if ((cp >= '0') && (cp <= '9'))
            return cp - '0';
        if ((cp >= 'a') && (cp <= 'f'))
            return cp - 'a' + 10;
        if ((cp >= 'A') && (cp <= 'F'))
            return cp - 'A' + 10;
        return -1;
    }

    inline int32_t Tokenizer::parse_dec(const char_t & ch)
    {
        const codepoint_t cp = ch.nCode;
        if ((cp >= '0') && (cp <= '9'))
            return cp - '0';
        return -1;
    }

    status_t Tokenizer::read_single_line_comment()
    {
        char_t ch;
        status_t res;

        while ((res = getch(ch)) == STATUS_OK)
        {
            if (ch.nCode == '\n')
                return STATUS_OK;
            if ((res = putch(ch)) != STATUS_OK)
                return res;
        }

        return (res == STATUS_EOF) ? STATUS_OK : res;
    }

    status_t Tokenizer::read_multi_line_comment()
    {
        char_t ch[2];
        status_t res;

        while ((res = getch(ch[0])) == STATUS_OK)
        {
            if (ch[0].nCode == '*')
            {
                if ((res = getch(ch[1])) != STATUS_OK)
                {
                    putch(ch[0]);
                    return res;
                }
                if (ch[1].nCode == '/')
                    return STATUS_OK;
                if ((res = ungetch(ch[1])) != STATUS_OK)
                    return res;
            }
            if ((res = putch(ch[0])) != STATUS_OK)
                return res;
        }

        return res;
    }

    status_t Tokenizer::read_hex_codepoint(codepoint_t & c, size_t digits)
    {
        char_t ch;
        status_t res;
        codepoint_t cp = 0;

        for (size_t i=0; i<digits; ++i)
        {
            if ((res = getch(ch)) != STATUS_OK)
                return res;

            const int32_t code = parse_hex(ch);
            if (code < 0)
                return STATUS_UNEXPECTED_CHAR;

            cp = (cp << 4) | code;
        }

        return STATUS_OK;
    }

    status_t Tokenizer::read_hex_codepoint(codepoint_t & c)
    {
        char_t ch;
        status_t res;
        codepoint_t cp = 0;

        for (size_t i=0; i < 8; ++i)
        {
            if ((res = getch(ch)) != STATUS_OK)
                return res;

            const int32_t code = parse_hex(ch);
            if (code < 0)
                return ((i > 0) && (ch.nCode == ';')) ? STATUS_OK : STATUS_UNEXPECTED_CHAR;

            cp = (cp << 4) | code;
        }

        // Require terminating semicolon
        if ((res = getch(ch)) != STATUS_OK)
            return res;

        return (ch.nCode == ';') ? STATUS_OK : STATUS_UNEXPECTED_CHAR;
    }

    status_t Tokenizer::read_dec_codepoint(codepoint_t & c)
    {
        char_t ch;
        status_t res;
        codepoint_t cp = 0;

        for (size_t i=0; cp <= UINT32_MAX / 10; ++i)
        {
            if ((res = getch(ch)) != STATUS_OK)
                return res;

            const int32_t code = parse_hex(ch);
            if (code < 0)
                return ((i > 0) && (ch.nCode == ';')) ? STATUS_OK : STATUS_UNEXPECTED_CHAR;

            cp = (cp * 10) + code;
        }

        // Require terminating semicolon
        if ((res = getch(ch)) != STATUS_OK)
            return res;

        return (ch.nCode == ';') ? STATUS_OK : STATUS_UNEXPECTED_CHAR;
    }

    status_t Tokenizer::read_character()
    {
        char_t ch;
        status_t res;

        if ((res = getch(ch)) != STATUS_OK)
            return res;

        if (ch.nCode == '\\')
        {
            // Escape sequence: one of \n \t \v \r \a \f \\ \' \" \xXX \uXXXX \UXXXXXXXX; \#DDDD; \XXXXX;
            if ((res = getch(ch)) != STATUS_OK)
                return res;

            switch (ch.nCode)
            {
                case 'n':   ch.nCode  = '\n'; break;
                case 't':   ch.nCode  = '\t'; break;
                case 'v':   ch.nCode  = '\v'; break;
                case 'r':   ch.nCode  = '\r'; break;
                case 'f':   ch.nCode  = '\f'; break;
                case 'a':   ch.nCode  = '\a'; break;
                case '\\':  ch.nCode  = '\\'; break;
                case '\'':  ch.nCode  = '\''; break;
                case '\"':  ch.nCode  = '\n'; break;
                case 'x':
                    if ((res = read_hex_codepoint(ch.nCode, 2)) != STATUS_OK)
                        return res;
                    break;
                case 'u':
                    if ((res = read_hex_codepoint(ch.nCode, 4)) != STATUS_OK)
                        return res;
                    break;
                case 'U':
                    if ((res = read_hex_codepoint(ch.nCode, 8)) != STATUS_OK)
                        return res;
                    break;
                case 'X':
                    if ((res = read_hex_codepoint(ch.nCode)) != STATUS_OK)
                        return res;
                    break;
                case '#':
                    if ((res = read_dec_codepoint(ch.nCode)) != STATUS_OK)
                        return res;
                    break;
                default:
                    return STATUS_UNEXPECTED_CHAR;
            }

            if ((res = putch(ch)) != STATUS_OK)
                return res;
        }
        else if ((res == putch(ch)) != STATUS_OK)
            return res;

        // Require closing '
        if ((res = getch(ch)) != STATUS_OK)
            return res;
        return (ch.nCode == '\'') ? STATUS_OK : STATUS_BAD_CHAR_LITERAL;
    }

    status_t Tokenizer::read_string()
    {
        char_t ch;
        status_t res;

        while ((res = getch(ch)) == STATUS_OK)
        {
            const codepoint_t cp = ch.nCode;

            // End of string?
            if (cp == '\"')
                return STATUS_OK;

            if (cp == '\\')
            {
                // Escape sequence: one of \n \t \v \r \a \f \\ \' \" \xXX \uXXXX \UXXXXXXXX; \#DDDD; \XXXXX;
                if ((res = getch(ch)) != STATUS_OK)
                    return res;

                switch (ch.nCode)
                {
                    case 'n':   ch.nCode  = '\n'; break;
                    case 't':   ch.nCode  = '\t'; break;
                    case 'v':   ch.nCode  = '\v'; break;
                    case 'r':   ch.nCode  = '\r'; break;
                    case 'f':   ch.nCode  = '\f'; break;
                    case 'a':   ch.nCode  = '\a'; break;
                    case '\\':  ch.nCode  = '\\'; break;
                    case '\'':  ch.nCode  = '\''; break;
                    case '\"':  ch.nCode  = '\n'; break;
                    case 'x':
                        if ((res = read_hex_codepoint(ch.nCode, 2)) != STATUS_OK)
                            return res;
                        break;
                    case 'u':
                        if ((res = read_hex_codepoint(ch.nCode, 4)) != STATUS_OK)
                            return res;
                        break;
                    case 'U':
                        if ((res = read_hex_codepoint(ch.nCode, 8)) != STATUS_OK)
                            return res;
                        break;
                    case 'X':
                        if ((res = read_hex_codepoint(ch.nCode)) != STATUS_OK)
                            return res;
                        break;
                    case '#':
                        if ((res = read_dec_codepoint(ch.nCode)) != STATUS_OK)
                            return res;
                        break;
                    default:
                        // Invalid escape sequence
                        if ((res = putch('\\')) != STATUS_OK)
                            return res;
                        break;
                }
            }
            else if (cp == '\n')
                return STATUS_UNEXPECTED_EOL;

            if ((res == putch(ch)) != STATUS_OK)
                return res;
        }

        return res;
    }

    template <typename ... Args>
    inline bool Tokenizer::lookup(char_t & ch, const TokenSet & allowed, Args && ... args)
    {
        if (!allowed.contains(kife::forward<Args>(args)...))
            return false;
        return getch(ch) == STATUS_OK;
    }

    status_t Tokenizer::get(kife::token_t & tok, const TokenSet & allowed)
    {
        if (!vBuffer)
            return STATUS_BAD_STATE;

        // There is pending token?
        if (sToken.bUnget)
        {
            tok.type        = sToken.enType;
            tok.data        = sToken.vBuffer;
            tok.line        = sToken.nLine;
            tok.column      = sToken.nColumn;

            return STATUS_OK;
        }

        status_t res;
        char_t ch[MAX_UNGETCH];

        // Get first character
        sToken.nBufSize     = 0;
        sToken.enType       = TT_UNEXPECTED;
        do
        {
            if ((res = getch(ch[0])) != STATUS_OK)
                return res;
        }
        while (is_blank(ch[0]));

        sToken.nLine        = ch[0].nLine;
        sToken.nColumn      = ch[0].nColumn;

        switch (ch[0].nCode)
        {
            case '+': // tokens: + ++ +=
                if (lookup(ch[1], allowed, TT_INCREMENT, TT_PLUS_ASSIGN))
                {
                    if ((ch[1].nCode == '+') && (allowed.contains(TT_INCREMENT)))
                    {
                        if ((res = putch(ch, 2)) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_INCREMENT;
                        break;
                    }
                    else if ((ch[1].nCode == '=') && (allowed.contains(TT_PLUS_ASSIGN)))
                    {
                        if ((res = putch(ch, 2)) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_PLUS_ASSIGN;
                        break;
                    }
                    if ((res = ungetch(ch[1])) != STATUS_OK)
                        return res;
                }
                if (allowed.contains(TT_PLUS))
                {
                    sToken.enType       = TT_PLUS;
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                }

                break;

            case '-': // tokens: - -- -> -=
                if (lookup(ch[1], allowed, TT_DECREMENT, TT_POINTER, TT_MINUS_ASSIGN))
                {
                    if ((ch[1].nCode == '-') && (allowed.contains(TT_DECREMENT)))
                    {
                        if ((res = putch(ch, 2)) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_DECREMENT;
                        break;
                    }
                    else if ((ch[1].nCode == '>') && (allowed.contains(TT_POINTER)))
                    {
                        if ((res = putch(ch, 2)) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_POINTER;
                        break;
                    }
                    else if ((ch[1].nCode == '=') && (allowed.contains(TT_MINUS_ASSIGN)))
                    {
                        if ((res = putch(ch, 2)) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_MINUS_ASSIGN;
                        break;
                    }
                    if ((res = ungetch(ch[1])) != STATUS_OK)
                        return res;
                }
                if (allowed.contains(TT_MINUS))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_MINUS;
                }
                break;

            case '/': // tokens: / /= // /*
                if (lookup(ch[1], allowed, TT_DIV_ASSIGN, TT_LINE_COMMENT, TT_MULTILINE_COMMENT))
                {
                    if ((ch[1].nCode == '=') && (allowed.contains(TT_DIV_ASSIGN)))
                    {
                        if ((res = putch(ch, 2)) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_DIV_ASSIGN;
                        break;
                    }
                    else if ((ch[1].nCode == '/') && (allowed.contains(TT_LINE_COMMENT)))
                    {
                        if ((res = read_single_line_comment()) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_LINE_COMMENT;
                        break;
                    }
                    else if ((ch[1].nCode == '*') && (allowed.contains(TT_MULTILINE_COMMENT)))
                    {
                        if ((res = read_multi_line_comment()) != STATUS_OK)
                        {
                            if (res == STATUS_EOF)
                            {
                                sToken.enType   = TT_EOF; // Unexpected end of file
                                break;
                            }
                            return res;
                        }
                        sToken.enType       = TT_MULTILINE_COMMENT;
                        break;
                    }
                    if ((res = ungetch(ch[1])) != STATUS_OK)
                        return res;
                }
                if (allowed.contains(TT_DIV))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_DIV;
                }
                break;

            case '*': // tokens: * *=
                if (lookup(ch[1], allowed, TT_MUL_ASSIGN))
                {
                    if (ch[1].nCode == '=')
                    {
                        if ((res = putch(ch, 2)) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_MUL_ASSIGN;
                        break;
                    }
                    if ((res = ungetch(ch[1])) != STATUS_OK)
                        return res;
                }
                if (allowed.contains(TT_MUL))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_MUL;
                }
                break;

            case '%': // tokens: % %=
                if (lookup(ch[1], allowed, TT_MOD_ASSIGN))
                {
                    if (ch[1].nCode == '=')
                    {
                        if ((res = putch(ch, 2)) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_MOD_ASSIGN;
                        break;
                    }
                    if ((res = ungetch(ch[1])) != STATUS_OK)
                        return res;
                }
                if (allowed.contains(TT_MOD))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_MOD;
                }
                break;

            case '(': // tokens: (
                if (allowed.contains(TT_LBRACKET))
                    sToken.enType       = TT_LBRACKET;
                break;

            case ')': // tokens: )
                if (allowed.contains(TT_RBRACKET))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_RBRACKET;
                }
                break;

            case '{': // tokens: {
                if (allowed.contains(TT_LBRACE))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_LBRACE;
                }
                break;

            case '}': // tokens: }
                if (allowed.contains(TT_RBRACE))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_RBRACE;
                }
                break;

            case '[': // tokens: [
                if (allowed.contains(TT_LQBRACKET))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_LQBRACKET;
                }
                break;

            case ']': // tokens: ]
                if (allowed.contains(TT_RQBRACKET))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_RQBRACKET;
                }
                break;

            case ':': // tokens: :
                if (allowed.contains(TT_COLON))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_COLON;
                }
                break;

            case ';': // tokens: ;
                if (allowed.contains(TT_SEMICOLON))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_SEMICOLON;
                }
                break;

            case ',': // tokens: ,
                if (allowed.contains(TT_COMMA))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_COMMA;
                }
                break;

            case '.': // tokens: .
                if (allowed.contains(TT_DOT))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_DOT;
                }
                break;

            case '~': // tokens: ~
                if (allowed.contains(TT_NEG))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_NEG;
                }
                break;

            case '?': // tokens: ? ?? ?. ?:
                if (lookup(ch[1], allowed, TT_DOUBLE_QUESTION, TT_OPTIONAL_COLON, TT_OPTIONAL_DOT))
                {
                    if ((ch[1].nCode == '?') && (allowed.contains(TT_DOUBLE_QUESTION)))
                    {
                        if ((res = putch(ch, 2)) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_DOUBLE_QUESTION;
                        break;
                    }
                    else if ((ch[1].nCode == '.') && (allowed.contains(TT_OPTIONAL_DOT)))
                    {
                        if ((res = putch(ch, 2)) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_OPTIONAL_DOT;
                        break;
                    }
                    else if ((ch[1].nCode == ':') && (allowed.contains(TT_OPTIONAL_COLON)))
                    {
                        if ((res = putch(ch, 2)) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_OPTIONAL_COLON;
                        break;
                    }
                    if ((res = ungetch(ch[1])) != STATUS_OK)
                        return res;
                }
                if (allowed.contains(TT_QUESTION))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_QUESTION;
                }
                break;

            case '=': // tokens: = ==
                if (lookup(ch[1], allowed, TT_EQUAL))
                {
                    if (ch[1].nCode == '=')
                    {
                        if ((res = putch(ch, 2)) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_EQUAL;
                        break;
                    }
                    if ((res = ungetch(ch[1])) != STATUS_OK)
                        return res;
                }
                if (allowed.contains(TT_ASSIGN))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_ASSIGN;
                }
                break;

            case '!': // tokens: ! !=
                if (lookup(ch[1], allowed, TT_NOT_EQUAL))
                {
                    if (ch[1].nCode == '=')
                    {
                        if ((res = putch(ch, 2)) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_NOT_EQUAL;
                        break;
                    }
                    if ((res = ungetch(ch[1])) != STATUS_OK)
                        return res;
                }
                if (allowed.contains(TT_NOT))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_NOT;
                }
                break;

            case '&': // tokens: & &= && &&=
                if (lookup(ch[1], allowed, TT_AND_ASSIGN, TT_LOG_AND, TT_LOG_AND_ASSIGN))
                {
                    if ((ch[1].nCode == '=') && (allowed.contains(TT_AND_ASSIGN)))
                    {
                        if ((res = putch(ch, 2)) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_AND_ASSIGN;
                        break;
                    }
                    else if (ch[1].nCode == '&')
                    {
                        if (lookup(ch[2], allowed, TT_LOG_AND_ASSIGN))
                        {
                            if (ch[2].nCode == '=')
                            {
                                if ((res = putch(ch, 3)) != STATUS_OK)
                                    return res;
                                sToken.enType       = TT_LOG_AND_ASSIGN;
                                break;
                            }

                            if ((res = ungetch(ch[2])) != STATUS_OK)
                                return res;
                        }

                        if (allowed.contains(TT_LOG_AND))
                        {
                            if ((res = putch(ch, 2)) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_LOG_AND;
                            break;
                        }
                    }
                    if ((res = ungetch(ch[1])) != STATUS_OK)
                        return res;
                }
                if (allowed.contains(TT_AND))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_AND;
                }
                break;

            case '|': // tokens: | |= || ||=
                if (lookup(ch[1], allowed, TT_OR_ASSIGN, TT_LOG_OR, TT_LOG_OR_ASSIGN))
                {
                    if ((ch[1].nCode == '=') && (allowed.contains(TT_OR_ASSIGN)))
                    {
                        if ((res = putch(ch, 2)) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_OR_ASSIGN;
                        break;
                    }
                    else if (ch[1].nCode == '|')
                    {
                        if (lookup(ch[2], allowed, TT_LOG_OR_ASSIGN))
                        {
                            if (ch[2].nCode == '=')
                            {
                                if ((res = putch(ch, 3)) != STATUS_OK)
                                    return res;
                                sToken.enType       = TT_LOG_OR_ASSIGN;
                                break;
                            }

                            if ((res = ungetch(ch[2])) != STATUS_OK)
                                return res;
                        }

                        if (allowed.contains(TT_LOG_OR))
                        {
                            if ((res = putch(ch, 2)) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_LOG_OR;
                            break;
                        }
                    }
                    if ((res = ungetch(ch[1])) != STATUS_OK)
                        return res;
                }
                if (allowed.contains(TT_OR))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_OR;
                }
                break;

            case '^': // tokens: ^ ^= ^^ ^^=
                if (lookup(ch[1], allowed, TT_XOR_ASSIGN, TT_LOG_XOR, TT_LOG_XOR_ASSIGN))
                {
                    if ((ch[1].nCode == '=') && (allowed.contains(TT_XOR_ASSIGN)))
                    {
                        if ((res = putch(ch, 2)) != STATUS_OK)
                            return res;
                        sToken.enType       = TT_XOR_ASSIGN;
                        break;
                    }
                    else if (ch[1].nCode == '^')
                    {
                        if (lookup(ch[2], allowed, TT_LOG_XOR_ASSIGN))
                        {
                            if (ch[2].nCode == '=')
                            {
                                if ((res = putch(ch, 3)) != STATUS_OK)
                                    return res;
                                sToken.enType       = TT_LOG_XOR_ASSIGN;
                                break;
                            }

                            if ((res = ungetch(ch[2])) != STATUS_OK)
                                return res;
                        }

                        if (allowed.contains(TT_LOG_XOR))
                        {
                            if ((res = putch(ch, 2)) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_LOG_XOR;
                            break;
                        }
                    }
                    if ((res = ungetch(ch[1])) != STATUS_OK)
                        return res;
                }
                if (allowed.contains(TT_XOR))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_XOR;
                }
                break;

            case '<': // tokens: < <= << <<= <=< <=> <=<=
                if (lookup(ch[1], allowed, TT_LESS_EQ, TT_SHL, TT_SHL_ASSIGN, TT_ROL, TT_THREE_WAY, TT_ROL_ASSIGN))
                {
                    if (ch[1].nCode == '=') // tokens: <= <=< <=> <=<=
                    {
                        if (lookup(ch[2], allowed, TT_ROL, TT_THREE_WAY, TT_ROL_ASSIGN))
                        {
                            if (ch[2].nCode == '<')
                            {
                                if (lookup(ch[3], allowed, TT_ROL_ASSIGN)) // tokens: <=< <=<=
                                {
                                    if (ch[3].nCode == '=')
                                    {
                                        if ((res = putch(ch, 4)) != STATUS_OK)
                                            return res;

                                        sToken.enType       = TT_ROL_ASSIGN;
                                        break;
                                    }

                                    if ((res = ungetch(ch[3])) != STATUS_OK)
                                        return res;
                                }

                                if (allowed.contains(TT_ROL))
                                {
                                    if ((res = putch(ch, 3)) != STATUS_OK)
                                        return res;
                                    sToken.enType       = TT_ROL;
                                }
                                break;
                            }
                            else if ((ch[2].nCode == '>') && (allowed.contains(TT_THREE_WAY)))
                            {
                                if ((res = putch(ch, 3)) != STATUS_OK)
                                    return res;
                                sToken.enType       = TT_THREE_WAY;
                                break;
                            }

                            if ((res = ungetch(ch[2])) != STATUS_OK)
                                return res;
                        }

                        if (allowed.contains(TT_LESS_EQ))
                        {
                            if ((res = putch(ch, 2)) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_LESS_EQ;
                        }
                        break;
                    }
                    else if (ch[1].nCode == '<') // tokens: << <<=
                    {
                        if (lookup(ch[2], allowed, TT_SHL_ASSIGN))
                        {
                            if (ch[2].nCode == '=')
                            {
                                if ((res = putch(ch, 3)) != STATUS_OK)
                                    return res;
                                sToken.enType       = TT_SHL_ASSIGN;
                                break;
                            }

                            if ((res = ungetch(ch[2])) != STATUS_OK)
                                return res;
                        }

                        if (allowed.contains(TT_SHL))
                        {
                            if ((res = putch(ch, 2)) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_SHL;
                            break;
                        }
                    }
                    if ((res = ungetch(ch[1])) != STATUS_OK)
                        return res;
                }
                if (allowed.contains(TT_LESS))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_LESS;
                }

                break;

            case '>': // tokens: > >> >>> >>>= >= >>= >=> >=>=
                if (lookup(ch[1], allowed, TT_GREATER_EQ, TT_SHR, TT_SHR_ASSIGN, TT_ROR, TT_USHR, TT_USHR_ASSIGN, TT_ROR_ASSIGN))
                {
                    if (ch[1].nCode == '=') // tokens: >= >=> >=>=
                    {
                        if (lookup(ch[2], allowed, TT_ROR, TT_ROR_ASSIGN))
                        {
                            if (ch[2].nCode == '>') // tokens: >=> >=>=
                            {
                                if (lookup(ch[3], allowed, TT_ROR_ASSIGN))
                                {
                                    if (ch[3].nCode == '=')
                                    {
                                        if ((res = putch(ch, 4)) != STATUS_OK)
                                            return res;

                                        sToken.enType       = TT_ROR_ASSIGN;
                                        break;
                                    }

                                    if ((res = ungetch(ch[3])) != STATUS_OK)
                                        return res;
                                }

                                if (allowed.contains(TT_ROR))
                                {
                                    if ((res = putch(ch, 3)) != STATUS_OK)
                                        return res;
                                    sToken.enType       = TT_ROR;
                                    break;
                                }
                            }

                            if ((res = ungetch(ch[2])) != STATUS_OK)
                                return res;
                        }

                        if (allowed.contains(TT_GREATER_EQ))
                        {
                            if ((res = putch(ch, 2)) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_GREATER_EQ;
                        }
                        break;
                    }
                    else if (ch[1].nCode == '>') // tokens: >> >>= >>> >>>=
                    {
                        if (lookup(ch[2], allowed, TT_SHR_ASSIGN, TT_USHR, TT_USHR_ASSIGN))
                        {
                            if (ch[2].nCode == '>') // tokens: >>> >>>=
                            {
                                if (lookup(ch[2], allowed, TT_USHR_ASSIGN))
                                {
                                    if (ch[3].nCode == '=')
                                    {
                                        if ((res = putch(ch, 4)) != STATUS_OK)
                                            return res;

                                        sToken.enType       = TT_USHR_ASSIGN;
                                        break;
                                    }

                                    if ((res = ungetch(ch[3])) != STATUS_OK)
                                        return res;
                                }

                                if ((res = putch(ch, 3)) != STATUS_OK)
                                    return res;
                                sToken.enType       = TT_USHR;
                                break;
                            }

                            if ((ch[2].nCode == '=') && (allowed.contains(TT_SHR_ASSIGN)))
                            {
                                if ((res = putch(ch, 3)) != STATUS_OK)
                                    return res;
                                sToken.enType       = TT_SHR_ASSIGN;
                                break;
                            }

                            if ((res = ungetch(ch[3])) != STATUS_OK)
                                return res;
                        }

                        if (allowed.contains(TT_SHR))
                        {
                            if ((res = putch(ch, 2)) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_SHR;
                            break;
                        }
                    }
                    if ((res = ungetch(ch[1])) != STATUS_OK)
                        return res;
                }
                if (allowed.contains(TT_GREATER))
                {
                    if ((res = putch(ch, 1)) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_GREATER;
                }

                break;

            case '\'':
                if (allowed.contains(TT_CHARACTER))
                {
                    if ((res = read_character()) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_CHARACTER;
                }
                break;

            case '\"':
                if (allowed.contains(TT_STRING))
                {
                    if ((res = read_string()) != STATUS_OK)
                        return res;
                    sToken.enType       = TT_STRING;
                }
                break;

            // Left:
//            TT_IDENTIFIER,          // Identifier
//            TT_NUMERIC,             // Numeric constant
//            TT_STRING,              // String literal
//            TT_CHARACTER,           // Character

            default:
                if ((res = putch(ch[0])) != STATUS_OK)
                    return res;
                break;
        }

        // Put end of token
        ch[0].nCode     = 0;
        if ((res = putch(ch[0])) != STATUS_OK)
            return res;

        tok.type        = sToken.enType;
        tok.data        = sToken.vBuffer;
        tok.line        = sToken.nLine;
        tok.column      = sToken.nColumn;

        return STATUS_OK;
    }

    void Tokenizer::unget()
    {
        sToken.bUnget       = true;
    }

} /* namespace kife */
