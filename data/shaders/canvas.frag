#version 450

layout (location = 0) in vec4 i_color;
layout (location = 1) in vec2 i_texcoord;

layout (location = 0) out vec4 o_color;

layout (set = 0, binding = 0) uniform sampler2D u_texture;

void main() {
    o_color = i_color * texture(u_texture, i_texcoord.st);
}
