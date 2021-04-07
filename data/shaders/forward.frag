// Copyright (C) 2018-2021 Mariusz Dzikowski (thecyste@gmail.com)
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 var_position;
layout (location = 1) in vec2 var_texcoord;
layout (location = 2) in vec3 var_normal;

layout (location = 0) out vec4 out_color;

void main() {
	out_color = vec4(1.0, 1.0, 1.0, 1.0);
}