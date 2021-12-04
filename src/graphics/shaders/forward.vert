#version 460
#extension ARB_separate_shader_objects : enable

// Input 

layout (location = 0) in vec3 i_position;

#ifdef HAS_NORMAL
layout (location = 1) in vec3 i_normal;
#endif

#ifdef HAS_TANGENT
layout (location = 2) in vec4 i_tangent;
#endif 

#ifdef HAS_TEXCOORD_0
layout (location = 3) in vec2 i_texcoord_0;
#endif

#ifdef HAS_TEXCOORD_1
layout (location = 4) in vec2 i_texcoord_0;
#endif

#ifdef HAS_COLOR
layout (location = 5) in vec4 i_color;
#endif

// Output

layout (location = 0) out vec3 o_position;

#ifdef HAS_NORMAL
#ifdef HAS_TANGENT
layout (location = 1) out mat4 o_tbn;
#else
layout (location = 1) out vec3 o_normal;
#endif
#endif

#ifdef HAS_TEXCOORD_0
layout (location = 3) out vec2 o_texcoord_0;
#endif

#ifdef HAS_TEXCOORD_1
layout (location = 4) out vec2 o_texcoord_0;
#endif

#ifdef HAS_COLOR
layout (location = 5) out vec4 o_color;
#endif

// Uniforms

layout (std140, set = 0, binding = 0) uniform main_data {
	mat4 view_matrix;
	mat4 projection_matrix;
	mat4 projection_view_matrix;
	mat4 inverse_projection_view_matrix;
	mat4 last_projection_view_matrix;
	vec4 camera_position;
} u_main_data;

struct instance_data {
	mat4 world_matrix;
	mat4 normal_matrix;
};

layout (std140, set = 0, binding = 1) readonly buffer instance_buffer {
	instance_data u_instances[];
};

invariant gl_Position;

void main() {
	o_position = (u_instances[gl_BaseInstance].world_matrix * vec4(i_position, 1.0)).xyz;

#ifdef HAS_NORMAL
#ifdef HAS_TANGENT
	vec3 normal = normalize(i_normal);
	vec3 tangent = normalize(i_tangent);
	vec3 normal_w = normalize(vec3(u_instances[gl_BaseInstance].normal_matrix * vec4(normal, 0.0)));
	vec3 tangent_w = normalize(vec3(u_instances[gl_BaseInstance].world_matrix * vec4(tangent, 0.0)));
	vec3 bitangent_w = cross(normal_w, tangent_w) * i_tangent.w;
	o_tbn = mat4(tangent_w, bitangent_w, normal_w);
#else
	o_normal = normalize(vec3(u_instances[gl_BaseInstance].normal_matrix * vec4(normalize(i_normal), 0.0));
#endif
#endif

#ifdef HAS_TEXCOORD_0
	o_texcoord_0 = i_texcoord_0;
#endif

#ifdef HAS_TEXCOORD_1
	o_texcoord_1 = i_texcoord_1;
#endif

#ifdef HAS_COLOR
	o_color = i_color;
#endif

	gl_Position = u_main_data.projection_view_matrix * u_instances[gl_BaseInstance].world_matrix * vec4(i_position, 1.0);
}
