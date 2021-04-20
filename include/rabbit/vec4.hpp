#pragma once 

namespace rb {
    template<typename T>
    struct vec4 {
        T x, y, z, w;
    };

    using vec4i = vec4<int>;
    using vec4f = vec4<float>;
}
