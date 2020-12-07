#include "standard_shaders_ogl3.hpp"

using namespace rb;

static const char* solid_pixel_shader = 
"#version 100\n"

"precision mediump float;\n"

"varying vec2 v_texcoord;\n"
"varying vec4 v_color;\n"

"void main() {\n"
"	gl_FragColor = v_color;\n"
"}"
;

static const char* texture_pixel_shader =
"#version 100\n"

"precision mediump float;\n"

"varying vec2 v_texcoord;\n"
"varying vec4 v_color;\n"

"uniform sampler2D u_texture;\n"

"void main() {\n"
"	gl_FragColor = texture2D(u_texture, v_texcoord) * v_color;\n"
"}\n"
;

static const char* vertex_shader = 
"#version 100\n"

"attribute vec3 in_position;\n"
"attribute vec2 in_texcoord;\n"
"attribute vec4 in_color;\n"

"uniform mat4 u_projection;\n"
"uniform mat4 u_view;\n"
"uniform mat4 u_world;\n"

"varying vec2 v_texcoord;\n"
"varying vec4 v_color;\n"

"void main() {\n"
"	v_texcoord = in_texcoord;\n"
"    v_color = in_color;\n"
"	gl_Position = u_projection * u_view * u_world * vec4(in_position.x, in_position.y, in_position.z, 1.0);\n"
"}\n"
;

const char* standard_shaders_ogl3::solid_pixel_shader() {
	return ::solid_pixel_shader;
}

const char* standard_shaders_ogl3::texture_pixel_shader() {
	return ::texture_pixel_shader;
}

const char* standard_shaders_ogl3::vertex_shader() {
	return ::vertex_shader;
}
