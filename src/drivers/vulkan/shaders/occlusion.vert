#version 450
#extension GL_ARB_shader_draw_parameters : enable

#ifndef VULKAN
#define gl_InstanceIndex gl_BaseInstance
#endif

#define THRESHOLD 1.1

struct world_data {
    mat4 transform;
    vec4 bsphere;
};

layout (location = 0) in vec3 i_position;

layout (std140, set = 0, binding = 0) uniform main_data {
    mat4 proj;
    mat4 view;
    mat4 proj_view;
    mat4 inv_proj_view;
    vec4 camera_position;
    vec4 camera_frustum;
    uint instance_count;
} u_main;

layout (std430, set = 0, binding = 1) readonly buffer world_buffer {
    world_data data[];
} u_world;

invariant gl_Position;

void main() {
    vec4 bsphere = u_world.data[gl_InstanceIndex].bsphere;
    vec3 position = normalize(i_position) * bsphere.w * THRESHOLD + bsphere.xyz;
    gl_Position = u_main.proj_view * u_world.data[gl_InstanceIndex].transform * vec4(position, 1.0);

#ifdef VULKAN
    gl_Position.y = -gl_Position.y;
#endif
}
