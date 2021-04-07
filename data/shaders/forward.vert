// Copyright (C) 2018-2021 Mariusz Dzikowski (thecyste@gmail.com)
#version 450

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texcoord;
layout (location = 2) in vec3 in_normal;

layout (std140, binding = 0) uniform Matrices {
    mat4 world;
    mat4 view;
    mat4 proj;
};

layout (location = 0) out vec3 var_position;
layout (location = 1) out vec2 var_texcoord;
layout (location = 2) out vec3 var_normal;

void main() {
	var_normal = (world * vec4(normalize(in_normal), 0.0)).xyz;
	var_texcoord = in_texcoord;
	var_position = vec3(world * vec4(in_position, 1.0));
    gl_Position = proj * view * vec4(var_position, 1.0);
}
