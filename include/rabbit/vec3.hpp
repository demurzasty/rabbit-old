#pragma once 

namespace rb {
    template<typename T>
    struct vec3 {
        T x, y, z;
    };

    using vec3i = vec3<int>;
    using vec3f = vec3<float>;
}
