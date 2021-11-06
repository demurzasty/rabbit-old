#version 450

layout (location = 0) in vec3 i_position;
layout (location = 1) in vec2 i_texcoord;
layout (location = 2) in vec3 i_normal;

layout (std140, set = 0, binding = 0) uniform camera_data {
    mat4 u_proj;
    mat4 u_view;
    mat4 u_inv_proj_view;
    vec3 u_camera_position;
};

layout (std140, push_constant) uniform local_data {
    mat4 u_world;
};

void main() {
    gl_Position = u_proj * u_view * u_world * vec4(i_position, 1.0);

#ifdef VULKAN
    gl_Position.y = -gl_Position.y;
#endif
}
