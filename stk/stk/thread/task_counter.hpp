//
//! Copyright � 2017
//! Brandon Kohn
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <geometrix/utility/assert.hpp>
#include <atomic>
#include <cstdint>
#include <thread>

namespace stk { namespace thread {

    class task_counter
    {
    public:

        task_counter(std::uint32_t = 0)
            : m_counter(0)
        {}

        //! 0 is the main thread. [1..nthreads] are the pool threads.
        void increment(std::uint32_t)
        {
            m_counter.fetch_add(1, std::memory_order_relaxed);
        }

        std::size_t count() const
        {
            return m_counter.load(std::memory_order_relaxed);
        }

        void reset()
        {
            m_counter.store(0, std::memory_order_relaxed);
        }

    private:

        std::atomic<std::uint32_t> m_counter;
    };

}}//! namespace stk::thread;
