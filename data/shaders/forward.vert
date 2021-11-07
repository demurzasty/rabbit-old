#version 450

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texcoord;
layout (location = 2) in vec3 in_normal;

layout (std140, set = 0, binding = 0) uniform CameraData {
    mat4 proj;
    mat4 view;
    vec3 u_camera_position;
};

struct Light {
	vec3 dir_or_pos;
	float radius;
	vec3 color;
	int type;
};

//layout (std140, set = 0, binding = 2) uniform LightListData {
//    Light lights[16];
//    mat4 light_proj_view;
//    int light_count;
//};

#ifdef VULKAN 
layout (std140, push_constant) uniform LocalData {
    mat4 world;
};
#else
layout (std140, binding = 1) uniform LocalData {
    mat4 world;
};
#endif

layout (location = 0) out vec3 v_position;
layout (location = 1) out vec2 v_texcoord;
layout (location = 2) out vec3 v_normal;

const mat4 bias_mat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

void main() {
    v_position = (world * vec4(in_position, 1.0)).xyz;
    v_texcoord = in_texcoord;
    v_normal = (world * vec4(normalize(in_normal), 0.0)).xyz;
    // v_shadow_coord = (light_proj_view * world) * vec4(in_position, 1.0);	
    gl_Position = proj * view * vec4(v_position, 1.0);

#ifdef VULKAN
    gl_Position.y = -gl_Position.y;
#endif
}
