#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 v_texcoord;

layout (location = 0) out vec4 o_color;

layout (std140, set = 0, binding = 0) uniform camera_data {
    mat4 u_proj;
    mat4 u_view;
    mat4 u_inv_proj_view;
    vec3 u_camera_position;
	mat4 u_light_proj_views[4];
    mat4 u_last_proj_view;
};

layout (set = 1, binding = 0) uniform sampler2D u_albedo_map;
layout (set = 1, binding = 1) uniform sampler2D u_normal_map;
layout (set = 1, binding = 2) uniform sampler2D u_emissive_map;
layout (set = 1, binding = 3) uniform sampler2D u_depth_map;

layout (set = 2, binding = 0) uniform sampler2D u_postprocess_map;

const int strength = 5;

vec3 extract_position(float depth, vec2 texcoord) {
    vec4 position;
	position.x = texcoord.x * 2.0 - 1.0;
    position.y = -(texcoord.y * 2.0 - 1.0);
	position.z = depth;
	position.w = 1.0;
	position = u_inv_proj_view * position;
	return position.xyz / position.w;
}

void main() {
    vec3 curr_postion = extract_position(texture(u_depth_map, v_texcoord).r, v_texcoord);
    vec4 last_position = u_last_proj_view * vec4(curr_postion, 1.0);
    last_position.xyz /= last_position.w;
    last_position.y = -last_position.y;
    last_position.xy = last_position.xy * 0.5 + 0.5;

    vec2 velocity = (v_texcoord - last_position.xy) / strength;

    vec3 color = vec3(0.0);
    for (int i = 0; i < strength; ++i) {
        color += texture(u_postprocess_map, v_texcoord + velocity.xy * i).rgb;
    }
    o_color = vec4(color / strength, 1.0);
}
