#version 450

layout (location = 0) in vec2 in_position;

layout (location = 0) out vec2 var_texcoord;

void main() {
	var_texcoord = in_position * 0.5 + 0.5;
    gl_Position = vec4(in_position, 0.0, 1.0);
}
