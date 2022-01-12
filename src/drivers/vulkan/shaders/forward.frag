#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec2 v_texcoord;
layout (location = 2) in vec3 v_normal;

struct command_data {
	uint index_count;
	uint instance_count;
	uint first_index;
	int vertex_offset;
	uint first_instance;
};

layout (std140, set = 0, binding = 0) uniform main_data {
    mat4 proj;
    mat4 view;
    mat4 proj_view;
    mat4 inv_proj_view;
    vec4 camera_position;
    vec4 camera_frustum;
    uint instance_count;
} u_main;

layout (set = 0, binding = 2) uniform sampler2D u_depth_pyramid;

layout (location = 0) out vec4 o_color;

vec3 extract_position(sampler2D depth_map, vec2 texcoord) {
    vec4 position;
	position.x = texcoord.x * 2.0 - 1.0;
    position.y = -(texcoord.y * 2.0 - 1.0);
	position.z = textureLod(depth_map, texcoord, 0).r;// * 2.0 - 1.0;
	position.w = 1.0;
	position = u_main.inv_proj_view * position;
	return position.xyz / position.w;
}

void main() {
	o_color = vec4(v_normal * 0.5 + 0.5, 1.0);

	//o_color = vec4(gl_FragCoord.xy / vec2(1280.0, 720.0), 0.0, 1.0);

	vec2 uv = gl_FragCoord.xy / vec2(1280.0, 720.0);
	vec3 position = extract_position(u_depth_pyramid, uv);

	vec4 test = u_main.proj_view * vec4(v_position, 1.0);
	test.xyz /= test.w;

	float depth = test.z;

	//o_color = vec4(textureLod(u_depth_pyramid, uv, 0).r / 10.0, 0.0, 0.0, 1.0);
	o_color = vec4(vec3(textureLod(u_depth_pyramid, uv, 4).r), 1.0);
	//o_color = vec4(vec3(depth * 0.5 + 0.5), 1.0);
}
