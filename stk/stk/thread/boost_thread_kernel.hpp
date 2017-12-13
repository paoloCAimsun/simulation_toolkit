//
//! Copyright © 2017
//! Brandon Kohn
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef STK_THREAD_BOOST_THREAD_TRAITS_HPP
#define STK_THREAD_BOOST_THREAD_TRAITS_HPP

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

namespace stk { namespace thread {

struct boost_thread_traits
{
    using thread_type = boost::thread;
    using mutex_type = boost::mutex;
    using recursive_mutex_type = boost::recursive_mutex;
    using shared_mutex_type = boost::shared_mutex;
    
    template <typename T>
    using future_type = boost::BOOST_THREAD_FUTURE<T>;
    
#if(BOOST_THREAD_VERSION < 4)
    template <typename T, typename...Ts>
    using packaged_task_type = boost::packaged_task<T>;
#else
    template <typename T, typename...Ts>
    using packaged_task_type = boost::packaged_task<T(Ts...)>;
#endif
    
    template <typename T>
    using promise_type = boost::promise<T>;
    
    static void interrupt(boost::thread& t){ t.interrupt(); }
    static void join(boost::thread& t){ t.join();}
    static void interruption_point() { boost::this_thread::interruption_point(); }
    
    static void yield() { boost::this_thread::yield(); }
};

}}//! namespace stk::thread;

#endif//! STK_THREAD_BOOST_THREAD_TRAITS_HPP
