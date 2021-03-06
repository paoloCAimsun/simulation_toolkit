//
//! Copyright © 2017
//! Brandon Kohn
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef STK_ORIENTED_BOUNDING_BOX_HPP
#define STK_ORIENTED_BOUNDING_BOX_HPP

#if defined(_MSC_VER)
    #pragma once
#endif

#include "point.hpp"
#include <stk/geometry/tensor/vector.hpp>

#include <geometrix/primitive/oriented_bounding_box.hpp>

namespace stk { 

using obb2 = geometrix::oriented_bounding_box<point2, dimensionless2>;

}//! namespace stk;

#endif//STK_ORIENTED_BOUNDING_BOX_HPP
