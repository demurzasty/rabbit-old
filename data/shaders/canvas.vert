#version 450

layout (location = 0) in vec2 i_position;
layout (location = 1) in vec2 i_texcoord;
layout (location = 2) in vec4 i_color;

layout (push_constant) uniform transform_data {
    vec2 scale;
    vec2 translate;
} u_transform;

layout (location = 0) out vec4 o_color;
layout (location = 1) out vec2 o_texcoord;

void main() {
    o_color = i_color;
    o_texcoord = i_texcoord;
    gl_Position = vec4(i_position * u_transform.scale + u_transform.translate, 0.0, 1.0);
}
