#version 450

layout (location = 0) in vec2 i_position;

layout (location = 0) out vec2 o_texcoord;

void main() {
	o_texcoord = i_position * 0.5 + 0.5;

	o_texcoord.y = 1.0 - o_texcoord.y;

    gl_Position = vec4(i_position, 0.0, 1.0);

#ifdef VULKAN
    gl_Position.y = -gl_Position.y;
#endif
}
