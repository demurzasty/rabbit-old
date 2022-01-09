#pragma once 

#include <algorithm>

namespace rb {
    template<size_t N>
    struct string_literal {
        constexpr string_literal(const char(&str)[N]) {
            std::copy_n(str, N, value);
        }

        char value[N];
    };
}
