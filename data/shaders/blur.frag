#version 450

layout (location = 0) in vec2 v_texcoord;

layout (location = 0) out vec4 o_color;

layout (set = 0, binding = 0) uniform sampler2D u_postprocess_map;

void main() {
    o_color = vec4(texture(u_postprocess_map, v_texcoord).rgb, 1.0);
}
