#version 450
#extension GL_ARB_shader_draw_parameters : enable

#ifndef VULKAN
#define gl_InstanceIndex gl_BaseInstance
#endif

struct world_data {
    mat4 transform;
    vec4 bsphere;
};

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texcoord;
layout (location = 2) in vec3 in_normal;

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

layout (location = 0) out vec3 v_position;
layout (location = 1) out vec2 v_texcoord;
layout (location = 2) out vec3 v_normal;

invariant gl_Position;

void main() {
    v_position = (u_world.data[gl_InstanceIndex].transform * vec4(in_position, 1.0)).xyz;
    v_texcoord = in_texcoord;
    v_normal = (u_world.data[gl_InstanceIndex].transform * vec4(normalize(in_normal), 0.0)).xyz;
    gl_Position = u_main.proj_view * u_world.data[gl_InstanceIndex].transform * vec4(in_position, 1.0);

#ifdef VULKAN
    gl_Position.y = -gl_Position.y;
#endif
}
