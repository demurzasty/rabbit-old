#version 450

layout (location = 0) in vec2 v_texcoord;

layout (location = 0) out vec4 o_color;

layout (set = 0, binding = 0) uniform sampler2D u_postprocess_map;

layout (std140, push_constant) uniform sharpen_data {
    float strength;
};

void main() {
    ivec2 texture_size = textureSize(u_postprocess_map, 0);
    vec2 texel = vec2(1.0 / texture_size.x, 1.0 / texture_size.y);

    vec3 color = vec3(0.0);
    
    vec3 up = texture(u_postprocess_map, v_texcoord + vec2(0.0, -1.0) * texel).rgb;
    vec3 left = texture(u_postprocess_map, v_texcoord + vec2(-1.0, 0.0) * texel).rgb;
    vec3 center = texture(u_postprocess_map, v_texcoord + vec2(0.0, 0.0) * texel).rgb;
    vec3 right = texture(u_postprocess_map, v_texcoord + vec2(1.0, 0.0) * texel).rgb;
    vec3 down = texture(u_postprocess_map, v_texcoord + vec2(0.0, 1.0) * texel).rgb;

    o_color = vec4((1.0 + 4.0 * strength) * center - strength * (up + left + right + down), 1.0);
}
