#version 450

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texcoord;
layout (location = 2) in vec3 in_normal;

layout (std140, set = 0, binding = 0) uniform camera {
    mat4 proj;
    mat4 view;
    mat4 inv_proj_view;
    vec3 camera_position;
	mat4 light_proj_views[4];
} u_camera;

layout (std140, push_constant) uniform local {
    mat4 world;
} u_local;

invariant gl_Position;

void main() {
    // TODO: Should be culled. 
	gl_Position =  u_camera.proj * u_camera.view * u_local.world * vec4(in_position, 1.0);
    
#ifdef VULKAN
    gl_Position.y = -gl_Position.y;
#endif
}
