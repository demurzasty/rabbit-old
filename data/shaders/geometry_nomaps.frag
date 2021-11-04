#version 450

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec2 v_texcoord;
layout (location = 2) in vec3 v_normal;

layout (location = 0) out vec4 o_albedo;
layout (location = 1) out vec4 o_normal;
layout (location = 2) out vec4 o_emissive;

layout (std140, set = 0, binding = 0) uniform camera_data {
    mat4 u_proj;
    mat4 u_view;
    mat4 u_inv_proj_view;
    vec3 u_camera_position;
};

layout (std140, set = 1, binding = 0) uniform material_data {
    vec3 u_base_color;
    float u_roughness;
    float u_metallic;
};

void main() {
    vec4 albedo = vec4(u_base_color, 1.0);
    vec3 normal = v_normal;
    float roughness = u_roughness;
    float metallic = u_metallic;

    o_albedo = vec4(albedo.rgb, roughness);
    o_normal = vec4(normal * 0.5 + 0.5, metallic);
    o_emissive = vec4(0.0, 0.0, 0.0, 1.0);
}
