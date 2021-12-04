#pragma once 

#include "../math/vec2.hpp"
#include "../math/vec3.hpp"

namespace rb {
	struct vertex {
		vec3f position;
		vec2f texcoord;
		vec3f normal;
	};
}
