#version 450

layout (location = 0) in vec2 v_texcoord;

layout (location = 0) out vec4 o_color;

layout (set = 0, binding = 0) uniform sampler2D u_postprocess_map;

layout (std140, push_constant) uniform blur_data {
    int strength;
};

void main() {
    ivec2 texture_size = textureSize(u_postprocess_map, 0);
    vec2 texel = vec2(1.0 / texture_size.x, 1.0 / texture_size.y);

    vec3 color = vec3(0.0);
    
    int count = (strength * 2 + 1) * (strength * 2 + 1);
    for (int x = -strength; x <= strength; ++x) {
        for (int y = -strength; y <= strength; ++y) {
            color += texture(u_postprocess_map, v_texcoord + vec2(x, y) * texel).rgb;
        }
    }

    o_color = vec4(color / count, 1.0);
}
