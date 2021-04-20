#pragma once 

namespace rb {
    template<typename T>
    struct vec2 {
        T x, y;
    };

    using vec2i = vec2<int>;
    using vec2f = vec2<float>;
}
