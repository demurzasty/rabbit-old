#version 450

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texcoord; // todo: ??
layout (location = 2) in vec3 in_normal;   // todo: ??

layout (std140, binding = 0) uniform Matrices {
    mat4 world;
    mat4 view;
    mat4 proj;
};

layout (location = 0) out vec3 var_texcoord;

void main() {
	var_texcoord = normalize(in_position);
	vec4 pos = proj * view * vec4(in_position, 1.0);
    gl_Position = pos.xyww;
}