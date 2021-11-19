#version 450

layout (location = 0) in vec3 i_position;
layout (location = 1) in vec2 i_texcoord;
layout (location = 2) in vec3 i_normal;

layout (std140, set = 0, binding = 0) uniform CameraData {
    mat4 proj;
    mat4 view;
    vec3 u_camera_position;
};

layout (std140, push_constant) uniform LocalData {
    mat4 world;
};

layout (location = 0) out vec3 v_position;
layout (location = 1) out vec2 v_texcoord;
layout (location = 2) out vec3 v_normal;

invariant gl_Position;

void main() {
    v_position = (world * vec4(i_position, 1.0)).xyz;
    v_texcoord = i_texcoord;
    v_normal = (world * vec4(normalize(i_normal), 0.0)).xyz;
    gl_Position = proj * view * world * vec4(i_position, 1.0);

#ifdef VULKAN
    gl_Position.y = -gl_Position.y;
#endif
}
