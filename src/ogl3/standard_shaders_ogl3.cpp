#include "standard_shaders_ogl3.hpp"

using namespace rb;

static const char* solid_pixel_shader = 
R"(#version 450
in vec2 v_texcoord;
in vec4 v_color;

layout(location = 0) out vec4 out_color;

void main() {
	out_color = v_color;
}
)";

static const char* texture_pixel_shader =
R"(#version 450
in vec2 v_texcoord;
in vec4 v_color;

uniform sampler2D u_texture;

layout(location = 0) out vec4 out_color;

void main() {
	out_color = texture(u_texture, v_texcoord) * v_color;
}
)";

static const char* vertex_shader = 
R"(#version 450
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texcoord;
layout(location = 2) in vec4 in_color;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_world;

out vec2 v_texcoord;
out vec4 v_color;

void main() {
	v_texcoord = in_texcoord;
    v_color = in_color;
	gl_Position = u_projection * u_view * u_world * vec4(in_position, 0.0, 1.0);
}
)";

const char* standard_shaders_ogl3::solid_pixel_shader() {
	return ::solid_pixel_shader;
}

const char* standard_shaders_ogl3::texture_pixel_shader() {
	return ::texture_pixel_shader;
}

const char* standard_shaders_ogl3::vertex_shader() {
	return ::vertex_shader;
}
