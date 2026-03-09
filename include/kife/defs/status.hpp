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

#ifndef KIFE_DEFS_STATUS_HPP_
#define KIFE_DEFS_STATUS_HPP_

namespace kife
{
    /**
     * Status code
     */
    typedef int                         status_t;

    enum status_codes_t
    {
        STATUS_OK,
        STATUS_NO_MEM,
        STATUS_EOF,
        STATUS_IO_ERROR,
        STATUS_BAD_STATE,
        STATUS_BAD_ARGUMENTS,
        STATUS_OVERFLOW,
    };

} /* namespace kife */


#endif /* KIFE_DEFS_STATUS_HPP_ */
