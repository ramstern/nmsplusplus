#pragma once

#include <type_traits>
#include <vector>
#include <string>

namespace traits_ext
{
    template<typename T>
    struct is_vector : std::false_type {};

    template<typename T, typename Alloc>
    struct is_vector<std::vector<T, Alloc>> : std::true_type {};

    template<typename T>
    struct is_basic_string : std::false_type {};

    template<typename CharT, typename Traits, typename Allocator>
    struct is_basic_string<std::basic_string<CharT, Traits, Allocator>> : std::true_type {};

    template<typename T, typename = std::void_t<>>
    struct has_value_type : std::false_type {};

    template<typename T>
    struct has_value_type<T, std::void_t<typename T::value_type>> : std::true_type {};

    template<typename T, bool HasValueType = has_value_type<T>::value>
    struct underlying_type {
        using type = T;
    };

    template<typename T>
    struct underlying_type<T, true> {
        // Check if T is std::basic_string to stop recursion
        static constexpr bool is_basic_str = is_basic_string<T>::value;

        using type = std::conditional_t<
            is_basic_str,
            T,  // Stop unwrapping if T is std::basic_string
            typename underlying_type<typename T::value_type>::type
        >;
    };
}