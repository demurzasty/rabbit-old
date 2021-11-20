#pragma once 

// TODO: Vertex should be separated into buffers.
//       We can then support bones and vertex color/second texcoord.

#include <rabbit/engine/math/vec2.hpp>
#include <rabbit/engine/math/vec3.hpp>
#include <rabbit/engine/graphics/color.hpp>

namespace rb {
	struct canvas_vertex {
		vec2f position;
		vec2f texcoord;
		color color;
	};

	struct vertex {
		vec3f position;
		vec2f texcoord;
		vec3f normal;
	};
}
