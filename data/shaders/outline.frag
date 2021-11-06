#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 v_texcoord;

layout (location = 0) out vec4 o_color;

layout (set = 0, binding = 0) uniform sampler2D u_fill_map;

layout (set = 1, binding = 0) uniform sampler2D u_postprocess_map;

const vec2 offsets[8] = {
	vec2(-1.0, 0.0),
	vec2(1.0, 0.0),
	vec2(0.0, -1.0),
	vec2(0.0, 1.0),

	vec2(-1.0, -1.0),
	vec2(1.0, -1.0),
	vec2(1.0, 1.0),
	vec2(-1.0, 1.0),
};

const int strength = 3;

void main() {
	ivec2 fill_map_size = textureSize(u_fill_map, 0);
	vec2 texel = vec2(1.0 / fill_map_size.x, 1.0 / fill_map_size.y);

	if (texture(u_fill_map, v_texcoord).r < 0.5) {
		for (int i = 0; i < 8; ++i) {
			for (int j = 0; j < strength; ++j) {
				if (texture(u_fill_map, v_texcoord + texel * offsets[i] * strength).r > 0.5) {
					o_color = vec4(vec3(215.0 / 255.0, 103.0 / 255.0, 83.0 / 255.0), 1.0);
					return;
				}
			}
		}
	}
	o_color = vec4(texture(u_postprocess_map, v_texcoord).rgb, 1.0);
}
