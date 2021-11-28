#version 450

#define ORIENTATION_HORIZONTAL 0
#define ORIENTATION_VERTICAL 1

layout (constant_id = 0) const int ORIENTATION = ORIENTATION_HORIZONTAL;

layout (location = 0) in vec2 v_texcoord;

layout (location = 0) out vec4 o_color;

layout (set = 0, binding = 0) uniform sampler2D u_postprocess_map;

layout (std140, push_constant) uniform blur_data {
    int strength;
};

void main() {
    vec2 texel = 1.0 / textureSize(u_postprocess_map, 0);

    vec3 color = vec3(0.0);
    
    int count = (strength * 2 + 1);
    for (int i = -strength; i <= strength; ++i) {
        if (ORIENTATION == ORIENTATION_HORIZONTAL) {
          color += texture(u_postprocess_map, v_texcoord + vec2(i, 0.0) * texel).rgb;
        } else {
          color += texture(u_postprocess_map, v_texcoord + vec2(0.0, i) * texel).rgb;
        }
    }
    o_color = vec4(color / count, 1.0);
}
