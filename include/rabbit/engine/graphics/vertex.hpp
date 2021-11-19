#pragma once 

// TODO: Vertex should be separated into buffers.
//       We can then support bones and vertex color/second texcoord.

#include <rabbit/engine/math/vec2.hpp>
#include <rabbit/engine/math/vec3.hpp>

namespace rb {
	struct vertex {
		vec3f position;
		vec2f texcoord;
		vec3f normal;
	};
}
