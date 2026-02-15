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

#ifndef KIFE_DEFS_FINALLY_HPP_
#define KIFE_DEFS_FINALLY_HPP_

#include <kife/defs/utility.hpp>

namespace kife
{
    template <class T>
    class FinallyExecutor
    {
        private:
            T       sActor;

        public:
            inline FinallyExecutor(T && actor): sActor(kife::move(actor)) {}
            inline FinallyExecutor(T & actor) : sActor(actor) {}
            ~FinallyExecutor()  { sActor(); }
    };

    struct FinallyExecutorInit
    {
        template <class T>
        inline FinallyExecutor<T> operator + (T & actor)
        {
            return FinallyExecutor<T>(actor);
        }

        template <class T>
        inline FinallyExecutor<T> operator + (T && actor)
        {
            return FinallyExecutor<T>(kife::move(actor));
        }
    };

} /* namespace kife */

#define kife_impl_finally2(id, prefix) prefix ## id ## __
#define kife_impl_finally1(id, prefix) kife_impl_finally2(id, prefix)
#define kife_impl_finally0(id) \
    auto kife_impl_finally1(id, kife_finally_function) = ::kife::FinallyExecutorInit{} + [&]() -> void

#define finally kife_impl_finally0(__COUNTER__)

#endif /* KIFE_DEFS_FINALLY_HPP_ */
