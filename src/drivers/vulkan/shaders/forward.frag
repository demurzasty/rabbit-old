#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec2 v_texcoord;
layout (location = 2) in vec3 v_normal;

layout (location = 0) out vec4 o_color;

void main() {
	o_color = vec4(v_texcoord, 0.0, 1.0);
}
