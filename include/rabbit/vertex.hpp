#pragma once 

#include "vec2.hpp"
#include "vec3.hpp"
#include "color.hpp"

namespace rb {
    struct vertex {
        vec3f position;
        vec2f texcoord;
        color color;
    };
}
