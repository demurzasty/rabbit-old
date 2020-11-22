#pragma once 

#include "vec2.hpp"
#include "color.hpp"

namespace rb {
    struct vertex2d {
        vec2f position;
        vec2f texcoord;
        color color;
    };
}
