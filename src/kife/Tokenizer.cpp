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

    status_t Tokenizer::putch(const char_t & c)
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

        if (sToken.nBufSize <= 0)
        {
            sToken.nLine        = c.nLine;
            sToken.nColumn      = c.nColumn;
        }

        sToken.vBuffer[sToken.nBufSize++]   = c.nCode;
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

        if ((res = putch(ch[0])) != STATUS_OK)
            return res;

        switch (ch[0].nCode)
        {
            case '+': // tokens: + ++ +=
                if (allowed.contains(TT_INCREMENT, TT_PLUS_ASSIGN))
                {
                    if ((res = getch(ch[1])) == STATUS_OK)
                    {
                        if ((ch[1].nCode == '+') && (allowed.contains(TT_INCREMENT)))
                        {
                            if ((res = putch(ch[1])) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_INCREMENT;
                            break;
                        }
                        else if ((ch[1].nCode == '=') && (allowed.contains(TT_PLUS_ASSIGN)))
                        {
                            if ((res = putch(ch[1])) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_PLUS_ASSIGN;
                            break;
                        }
                        if ((res = ungetch(ch[1])) != STATUS_OK)
                            return res;
                    }
                }
                if (allowed.contains(TT_PLUS))
                {
                    sToken.enType       = TT_PLUS;
                    break;
                }
                break;

            case '-': // tokens: - -- -> -=
                if (allowed.contains(TT_DECREMENT, TT_POINTER, TT_MINUS_ASSIGN))
                {
                    if ((res = getch(ch[1])) == STATUS_OK)
                    {
                        if ((ch[1].nCode == '-') && (allowed.contains(TT_DECREMENT)))
                        {
                            if ((res = putch(ch[1])) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_DECREMENT;
                            break;
                        }
                        else if ((ch[1].nCode == '>') && (allowed.contains(TT_POINTER)))
                        {
                            if ((res = putch(ch[1])) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_POINTER;
                            break;
                        }
                        else if ((ch[1].nCode == '=') && (allowed.contains(TT_MINUS_ASSIGN)))
                        {
                            if ((res = putch(ch[1])) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_MINUS_ASSIGN;
                            break;
                        }
                        if ((res = ungetch(ch[1])) != STATUS_OK)
                            return res;
                    }
                }
                if (allowed.contains(TT_MINUS))
                    sToken.enType       = TT_MINUS;
                break;

            case '/': // tokens: / /= // /*
                if (allowed.contains(TT_DIV_ASSIGN, TT_LINE_COMMENT, TT_MULTILINE_COMMENT))
                {
                    if ((res = getch(ch[1])) == STATUS_OK)
                    {
                        if ((ch[1].nCode == '=') && (allowed.contains(TT_DIV_ASSIGN)))
                        {
                            if ((res = putch(ch[1])) != STATUS_OK)
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
                }
                if (allowed.contains(TT_DIV))
                    sToken.enType       = TT_DIV;
                break;

            case '*': // tokens: * *=
                if (allowed.contains(TT_MUL_ASSIGN))
                {
                    if ((res = getch(ch[1])) == STATUS_OK)
                    {
                        if (ch[1].nCode == '=')
                        {
                            if ((res = putch(ch[1])) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_MUL_ASSIGN;
                            break;
                        }
                        if ((res = ungetch(ch[1])) != STATUS_OK)
                            return res;
                    }
                }
                if (allowed.contains(TT_MUL))
                    sToken.enType       = TT_MUL;
                break;

            case '%': // tokens: % %=
                if (allowed.contains(TT_MOD_ASSIGN))
                {
                    if ((res = getch(ch[1])) == STATUS_OK)
                    {
                        if (ch[1].nCode == '=')
                        {
                            if ((res = putch(ch[1])) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_MOD_ASSIGN;
                            break;
                        }
                        if ((res = ungetch(ch[1])) != STATUS_OK)
                            return res;
                    }
                }
                if (allowed.contains(TT_MOD))
                    sToken.enType       = TT_MOD;
                break;

            case '(': // tokens: (
                if (allowed.contains(TT_LBRACKET))
                    sToken.enType       = TT_LBRACKET;
                break;

            case ')': // tokens: )
                if (allowed.contains(TT_RBRACKET))
                    sToken.enType       = TT_RBRACKET;
                break;

            case '{': // tokens: {
                if (allowed.contains(TT_LBRACE))
                    sToken.enType       = TT_LBRACE;
                break;

            case '}': // tokens: }
                if (allowed.contains(TT_RBRACE))
                    sToken.enType       = TT_RBRACE;
                break;

            case '[': // tokens: [
                if (allowed.contains(TT_LQBRACKET))
                    sToken.enType       = TT_LQBRACKET;
                break;

            case ']': // tokens: ]
                if (allowed.contains(TT_RQBRACKET))
                    sToken.enType       = TT_RQBRACKET;
                break;

            case ':': // tokens: :
                if (allowed.contains(TT_COLON))
                    sToken.enType       = TT_COLON;
                break;

            case ';': // tokens: ;
                if (allowed.contains(TT_SEMICOLON))
                    sToken.enType       = TT_SEMICOLON;
                break;

            case ',': // tokens: ,
                if (allowed.contains(TT_COMMA))
                    sToken.enType       = TT_COMMA;
                break;

            case '.': // tokens: .
                if (allowed.contains(TT_DOT))
                    sToken.enType       = TT_DOT;
                break;

            case '?': // tokens: ? ?? ?. ?:
                if (allowed.contains(TT_DOUBLE_QUESTION, TT_OPTIONAL_COLON, TT_OPTIONAL_DOT))
                {
                    if ((res = getch(ch[1])) == STATUS_OK)
                    {
                        if ((ch[1].nCode == '?') && (allowed.contains(TT_DOUBLE_QUESTION)))
                        {
                            if ((res = putch(ch[1])) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_DOUBLE_QUESTION;
                            break;
                        }
                        else if ((ch[1].nCode == '.') && (allowed.contains(TT_OPTIONAL_DOT)))
                        {
                            if ((res = putch(ch[1])) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_OPTIONAL_DOT;
                            break;
                        }
                        else if ((ch[1].nCode == ':') && (allowed.contains(TT_OPTIONAL_COLON)))
                        {
                            if ((res = putch(ch[1])) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_OPTIONAL_COLON;
                            break;
                        }
                        if ((res = ungetch(ch[1])) != STATUS_OK)
                            return res;
                    }
                }
                if (allowed.contains(TT_QUESTION))
                {
                    sToken.enType       = TT_QUESTION;
                    break;
                }
                break;

            case '=': // tokens: = ==
                if (allowed.contains(TT_EQUAL))
                {
                    if ((res = getch(ch[1])) == STATUS_OK)
                    {
                        if (ch[1].nCode == '=')
                        {
                            if ((res = putch(ch[1])) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_EQUAL;
                            break;
                        }
                        if ((res = ungetch(ch[1])) != STATUS_OK)
                            return res;
                    }
                }
                if (allowed.contains(TT_ASSIGN))
                {
                    sToken.enType       = TT_ASSIGN;
                    break;
                }
                break;

            case '!': // tokens: ! !=
                if (allowed.contains(TT_NOT_EQUAL))
                {
                    if ((res = getch(ch[1])) == STATUS_OK)
                    {
                        if (ch[1].nCode == '=')
                        {
                            if ((res = putch(ch[1])) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_NOT_EQUAL;
                            break;
                        }
                        if ((res = ungetch(ch[1])) != STATUS_OK)
                            return res;
                    }
                }
                if (allowed.contains(TT_NOT))
                {
                    sToken.enType       = TT_NOT;
                    break;
                }
                break;

            case '&': // tokens: & &= && &&=
                if (allowed.contains(TT_AND_ASSIGN, TT_LOG_AND, TT_LOG_AND_ASSIGN))
                {
                    if ((res = getch(ch[1])) == STATUS_OK)
                    {
                        if ((ch[1].nCode == '=') && (allowed.contains(TT_AND_ASSIGN)))
                        {
                            if ((res = putch(ch[1])) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_AND_ASSIGN;
                            break;
                        }
                        else if ((ch[1].nCode == '&') && (allowed.contains(TT_LOG_AND, TT_LOG_AND_ASSIGN)))
                        {
                            if ((res = getch(ch[2])) == STATUS_OK)
                            {
                                if ((ch[2].nCode == '=') && (allowed.contains(TT_LOG_AND_ASSIGN)))
                                {
                                    if ((res = putch(ch[2])) != STATUS_OK)
                                        return res;
                                    sToken.enType       = TT_LOG_AND_ASSIGN;
                                    break;
                                }
                            }

                            if ((res = ungetch(ch[2])) != STATUS_OK)
                                return res;

                            sToken.enType       = TT_LOG_AND;
                            break;
                        }
                        if ((res = ungetch(ch[1])) != STATUS_OK)
                            return res;
                    }
                }
                if (allowed.contains(TT_AND))
                {
                    sToken.enType       = TT_AND;
                    break;
                }
                break;

            case '|': // tokens: | |= || ||=
                if (allowed.contains(TT_OR_ASSIGN, TT_LOG_OR, TT_LOG_OR_ASSIGN))
                {
                    if ((res = getch(ch[1])) == STATUS_OK)
                    {
                        if ((ch[1].nCode == '=') && (allowed.contains(TT_OR_ASSIGN)))
                        {
                            if ((res = putch(ch[1])) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_OR_ASSIGN;
                            break;
                        }
                        else if ((ch[1].nCode == '|') && (allowed.contains(TT_LOG_OR, TT_LOG_OR_ASSIGN)))
                        {
                            if ((res = getch(ch[2])) == STATUS_OK)
                            {
                                if ((ch[2].nCode == '=') && (allowed.contains(TT_LOG_OR_ASSIGN)))
                                {
                                    if ((res = putch(ch[2])) != STATUS_OK)
                                        return res;
                                    sToken.enType       = TT_LOG_OR_ASSIGN;
                                    break;
                                }
                            }

                            if ((res = ungetch(ch[2])) != STATUS_OK)
                                return res;

                            sToken.enType       = TT_LOG_OR;
                            break;
                        }
                        if ((res = ungetch(ch[1])) != STATUS_OK)
                            return res;
                    }
                }
                if (allowed.contains(TT_OR))
                {
                    sToken.enType       = TT_OR;
                    break;
                }
                break;

            case '^': // tokens: ^ ^= ^^ ^^=
                if (allowed.contains(TT_XOR_ASSIGN, TT_LOG_XOR, TT_LOG_XOR_ASSIGN))
                {
                    if ((res = getch(ch[1])) == STATUS_OK)
                    {
                        if ((ch[1].nCode == '=') && (allowed.contains(TT_XOR_ASSIGN)))
                        {
                            if ((res = putch(ch[1])) != STATUS_OK)
                                return res;
                            sToken.enType       = TT_XOR_ASSIGN;
                            break;
                        }
                        else if ((ch[1].nCode == '^') && (allowed.contains(TT_LOG_XOR, TT_LOG_XOR_ASSIGN)))
                        {
                            if ((res = getch(ch[2])) == STATUS_OK)
                            {
                                if ((ch[2].nCode == '=') && (allowed.contains(TT_LOG_XOR_ASSIGN)))
                                {
                                    if ((res = putch(ch[2])) != STATUS_OK)
                                        return res;
                                    sToken.enType       = TT_LOG_XOR_ASSIGN;
                                    break;
                                }
                            }

                            if ((res = ungetch(ch[2])) != STATUS_OK)
                                return res;

                            sToken.enType       = TT_LOG_XOR;
                            break;
                        }
                        if ((res = ungetch(ch[1])) != STATUS_OK)
                            return res;
                    }
                }
                if (allowed.contains(TT_XOR))
                {
                    sToken.enType       = TT_XOR;
                    break;
                }
                break;

            // Left:
//            TT_LESS,                // Less: <
//            TT_LESS_EQ,             // Less or equal: <=
//            TT_SHL,                 // Left shift: <<
//            TT_SHL_ASSIGN,          // Left shift-assign: <<=
//            TT_ROL,                 // Cyclic left shift rotation: <=<
//            TT_ROL_ASSIGN,          // Cyclic left shift rotation-assign: <=<=
//            TT_THREE_WAY,           // Three way comparison: <=>
//            TT_GREATER,             // Greater: >
//            TT_GREATER_EQ,          // Greater or equal: >=
//            TT_SHR,                 // Right shift: >>
//            TT_SHR_ASSIGN,          // Right shift-assign: >>=
//            TT_ROR,                 // Cyclic right shift rotation: >=>
//            TT_ROR_ASSIGN,          // Cyclic right shift rotation-assign: >=>=
//            TT_USHR,                // Unsigned right shift: >>>
//            TT_USHR_ASSIGN,         // Unsigned right shift-assign: >>>=
//            TT_IDENTIFIER,          // Identifier
//            TT_NUMERIC,             // Numeric constant
//            TT_STRING,              // String literal
//            TT_CHARACTER,           // Character

            default:
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
