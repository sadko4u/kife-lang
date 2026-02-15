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
    static const char *memory_file_name="<memory>";

    Tokenizer::Tokenizer()
    {
        vBuffer         = nullptr;
        nBufSize        = 0;
        nOffset         = 0;
        pFD             = NULL;
        bClose          = false;

        sChar.nCode     = 0;
        sChar.bUnget    = false;

        sToken.enType   = TT_UNKNOWN;
        sToken.vBuffer  = nullptr;
        sToken.nBufCap  = 0;
        sToken.nBufSize = 0;
        sToken.sPath    = nullptr;
        sToken.nLine    = 1;
        sToken.nColumn  = 1;
        sToken.bUnget   = false;
    }

    Tokenizer::Tokenizer(Tokenizer && src)
    {
        vBuffer         = release_ptr(src.vBuffer);
        nBufSize        = kife::exchange(src.nBufSize, 0);
        nOffset         = kife::exchange(src.nOffset, 0);
        pFD             = release_ptr(src.pFD);
        bClose          = src.bClose;

        sChar           = kife::move(src.sChar);

        sToken.enType   = src.sToken.enType;
        sToken.vBuffer  = release_ptr(src.sToken.vBuffer);
        sToken.nBufCap  = kife::exchange(src.sToken.nBufCap, 0);
        sToken.nBufSize = kife::exchange(src.sToken.nBufSize, 0);
        sToken.sPath    = release_ptr(src.sToken.sPath);
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
        pFD             = release_ptr(src.pFD);
        bClose          = src.bClose;

        sChar           = kife::move(src.sChar);

        sToken.enType   = src.sToken.enType;
        sToken.vBuffer  = release_ptr(src.sToken.vBuffer);
        sToken.nBufCap  = kife::exchange(src.sToken.nBufCap, 0);
        sToken.nBufSize = kife::exchange(src.sToken.nBufSize, 0);
        sToken.sPath    = release_ptr(src.sToken.sPath);
        sToken.nLine    = 1;
        sToken.nColumn  = 1;
        sToken.bUnget   = false;

        return *this;
    }

    status_t Tokenizer::getch()
    {
        // Char has been unget?
        if (sChar.bUnget)
        {
            sChar.bUnget        = false;
            return sChar.nCode;
        }

        // Have enough data in the buffer?
        if (nOffset < nBufSize)
            return sChar.nCode = vBuffer[nOffset++];

        // Can fill the buffer ?
        if (pFD == NULL)
            return STATUS_EOF;

        nBufSize = fread(vBuffer, BUFFER_SIZE, 1, pFD);
        if (nBufSize > 0)
        {
            nOffset     = 1;
            return sChar.nCode = vBuffer[0];
        }

        return (feof(pFD)) ? STATUS_EOF : STATUS_IO_ERROR;
    }

    inline void Tokenizer::ungetch()
    {
        sChar.bUnget        = true;
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

    status_t Tokenizer::wrap(FILE *fd, bool close, const char *path)
    {
        if (!fd)
            return STATUS_BAD_ARGUMENTS;

        // Allocate I/O buffer
        char *iobuf         = static_cast<char *>(malloc(BUFFER_SIZE));
        if (!iobuf)
            return STATUS_NO_MEM;
        finally { free(iobuf);      };

        // Allocate token buffer
        char *tokbuf        = static_cast<char *>(malloc(BUFFER_SIZE));
        if (!tokbuf)
            return STATUS_NO_MEM;
        finally { free(tokbuf);     };

        // Remember file name
        char *name          = strdup((path) ? path : memory_file_name);
        if (!name)
            return STATUS_NO_MEM;
        finally { free(name);       };

        // Commit state
        vBuffer         = release_ptr(iobuf);
        nBufSize        = 0;
        nOffset         = 0;
        pFD             = fd;
        bClose          = close;

        sChar.nCode     = 0;
        sChar.bUnget    = false;

        sToken.enType   = TT_UNKNOWN;
        sToken.vBuffer  = release_ptr(tokbuf);
        sToken.nBufCap  = BUFFER_SIZE;
        sToken.nBufSize = 0;
        sToken.nLine    = 1;
        sToken.nColumn  = 1;
        sToken.sPath    = release_ptr(name);
        sToken.bUnget   = false;

        return STATUS_OK;
    }

    status_t Tokenizer::wrap(const void *buf, size_t count, bool free, const char *path)
    {
        if (!buf)
            return STATUS_BAD_ARGUMENTS;

        // Allocate token buffer
        char *tokbuf        = static_cast<char *>(malloc(BUFFER_SIZE));
        if (!tokbuf)
            return STATUS_NO_MEM;
        finally { ::free(tokbuf);   };

        // Remember file name
        char *name          = strdup((path) ? path : memory_file_name);
        if (!name)
            return STATUS_NO_MEM;
        finally { ::free(name);     };

        // Commit state
        vBuffer         = const_cast<char *>(static_cast<const char *>(buf));
        nBufSize        = count;
        nOffset         = 0;
        pFD             = nullptr;
        bClose          = free;

        sChar.nCode     = 0;
        sChar.bUnget    = false;

        sToken.enType   = TT_UNKNOWN;
        sToken.vBuffer  = release_ptr(tokbuf);
        sToken.nBufCap  = BUFFER_SIZE;
        sToken.nBufSize = 0;
        sToken.nLine    = 1;
        sToken.nColumn  = 1;
        sToken.sPath    = release_ptr(name);
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
        free(sToken.sPath);

        bClose          = false;
        vBuffer         = nullptr;
        sToken.vBuffer  = nullptr;
        sToken.sPath    = nullptr;

        return STATUS_OK;
    }

    status_t Tokenizer::get(token_t & tok, const TokenSet & allowed)
    {
        if (!vBuffer)
            return STATUS_BAD_STATE;

        // There is pending token?
        if (sToken.bUnget)
        {
            tok     = sToken;
            return STATUS_OK;
        }

        // Main logic

        return STATUS_OK;
    }

    void Tokenizer::unget()
    {
        sToken.bUnget       = true;
    }

} /* namespace kife */
