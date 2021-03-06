//
//! Copyright © 2017
//! Brandon Kohn
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef STK_RECTANGLE_HPP
#define STK_RECTANGLE_HPP

#if defined(_MSC_VER)
    #pragma once
#endif

#include "point.hpp"

#include <geometrix/primitive/rectangle.hpp>

namespace stk {

using rectangle2 = geometrix::rectangle<point2>;

}//! namespace stk;

#endif//STK_RECTANGLE_HPP
