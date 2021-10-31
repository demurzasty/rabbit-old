#version 450

layout (location = 0) in vec2 v_texcoord;

layout (location = 0) out vec4 o_color;

layout (set = 0, binding = 0) uniform sampler2D u_postprocess_map;
layout (set = 1, binding = 0) uniform sampler2D u_ssao_map;

layout (std140, push_constant) uniform blur_data {
    int strength;
};

void main() {
    ivec2 texture_size = textureSize(u_ssao_map, 0);
    vec2 texel = vec2(1.0 / texture_size.x, 1.0 / texture_size.y);

    float occlusion = 0.0;
    
    int count = (strength * 2 + 1) * (strength * 2 + 1);
    for (int x = -strength; x <= strength; ++x) {
        for (int y = -strength; y <= strength; ++y) {
            occlusion += texture(u_ssao_map, v_texcoord + vec2(x, y) * texel).r;
        }
    }

    o_color = vec4(vec3(occlusion / count), 1.0);
    // o_color = vec4(vec3(texture(u_ssao_map, v_texcoord).r), 1.0);
    o_color = vec4(texture(u_postprocess_map, v_texcoord).rgb * vec3(occlusion / count), 1.0);
}
