#pragma once

#include <type_traits>

namespace stk {
    struct none_type
    {
        struct init{};
        explicit none_type(init){}
    };
    using none_t = none_type;
    const none_type none = none_type{none_type::init()};

    template <typename T, typename EnableIf=void>
    struct is_none : std::false_type{};
    
    template <typename T>
    struct is_none
    <
        T
      , typename std::enable_if
        <
            std::is_same<typename std::decay<T>::type, none_type>::value
        >::type
    > : std::true_type
    {};

}//! namespace stk;

