#pragma once 

#include "vec2.hpp"
#include "vec3.hpp"
#include "color.hpp"

namespace rb {
    struct vertex {
        rb::vec3f position;
        rb::vec2f texcoord;
        rb::color color;
    };
}
