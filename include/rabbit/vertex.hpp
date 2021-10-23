#pragma once 

#include "vec2.hpp"
#include "vec3.hpp"

namespace rb {
	struct vertex {
		vec3f position;
		vec2f texcoord;
		vec3f normal;
	};
}
