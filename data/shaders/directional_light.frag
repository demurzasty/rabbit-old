#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define PI 3.14159265359

layout (location = 0) in vec2 v_texcoord;

layout (location = 0) out vec4 o_color;

layout (std140, set = 0, binding = 0) uniform camera_data {
    mat4 proj;
    mat4 view;
    mat4 u_inv_proj_view;
    vec3 u_camera_position;
};

layout(set = 0, binding = 1) uniform sampler2D u_brdf_map;
layout(set = 0, binding = 3) uniform sampler2D u_shadow_map;

layout (set = 1, binding = 0) uniform sampler2D u_albedo_map;
layout (set = 1, binding = 1) uniform sampler2D u_normal_map;
layout (set = 1, binding = 2) uniform sampler2D u_emissive_map;
layout (set = 1, binding = 3) uniform sampler2D u_depth_map;

layout(set = 2, binding = 0) uniform samplerCube u_radiance_map;
layout(set = 2, binding = 1) uniform samplerCube u_irradiance_map;
layout(set = 2, binding = 2) uniform samplerCube u_prefilter_map;

layout (std140, push_constant) uniform light_data {
    vec3 u_light_dir;
    vec3 u_light_color; // precomputed intensity in color
    mat4 u_light_proj_view;
};

float distribution_ggx(vec3 n, vec3 h, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float n_dot_h = max(dot(n, h), 0.0);
    float n_dot_h2 = n_dot_h * n_dot_h;

    float nom   = a2;
    float denom = (n_dot_h2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float geometry_schlick_ggx(float n_dot_v, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = n_dot_v;
    float denom = n_dot_v * (1.0 - k) + k;

    return nom / denom;
}

float geometry_smith(vec3 n, vec3 v, vec3 l, float roughness) {

    float n_dot_v = max(dot(n, v), 0.0);
    float n_dot_l = max(dot(n, l), 0.0);
    float ggx2 = geometry_schlick_ggx(n_dot_v, roughness);
    float ggx1 = geometry_schlick_ggx(n_dot_l, roughness);

    return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cos_theta, vec3 f0) {
    return f0 + (1.0 - f0) * pow(1.0 - cos_theta, 5.0);
}

vec3 extract_position(sampler2D depth_map, vec2 texcoord) {
    vec4 position;
	position.x = texcoord.x * 2.0 - 1.0;
    position.y = -(texcoord.y * 2.0 - 1.0);
	position.z = texture(depth_map, texcoord).r;// * 2.0 - 1.0;
	position.w = 1.0;
	position = u_inv_proj_view * position;
	return position.xyz / position.w;
}

float compute_shadow(in vec3 position) {
    vec4 shadow_position = u_light_proj_view * vec4(position, 1.0); // (v_shadow_coord.xyz / v_shadow_coord.w);
    shadow_position.xyz = shadow_position.xyz / shadow_position.w;

    vec2 shadow_coord = vec2(shadow_position.x, shadow_position.y) * 0.5 + 0.5;
    if (shadow_coord.x < 0.0 || shadow_coord.x > 1.0 ||
        shadow_coord.y < 0.0 || shadow_coord.y > 1.0) {
        return 1.0;
    }

    return step(shadow_position.z, texture(u_shadow_map, shadow_coord).x);
}

void main() {
    vec4 albedo_data = texture(u_albedo_map, v_texcoord);
    vec4 normal_data = texture(u_normal_map, v_texcoord);

    vec3 position = extract_position(u_depth_map, v_texcoord);
    vec3 albedo = albedo_data.rgb;
    vec3 normal = normalize(normal_data.xyz * 2.0 - 1.0);
    float roughness = albedo_data.a;
    float metallic = normal_data.a;
    
    vec3 v = normalize(u_camera_position - position);
    float n_dot_v = abs(dot(normal, v)) + 1e-5;

    vec3 f0 = mix(vec3(0.04), albedo, metallic);

    vec3 lo = vec3(0.0);
    {
        vec3 radiance = u_light_color;
        float intensity = 1.0;

        vec3 l = -normalize(u_light_dir);

        vec3 h = normalize(v + l);
        vec3 f = fresnel_schlick(max(dot(h, v), 0.0), f0);

        vec3 ks = f;
        vec3 kd = (1.0 - ks) * (1.0 - metallic);

        float ndf = distribution_ggx(normal, h, roughness);
        float g = geometry_smith(normal, v, l, roughness);

        float n_dot_l = max(dot(normal, l), 0.0);

        vec3 nominator = ndf * g * f;
        float denominator = 4.0 * n_dot_v * n_dot_l + 0.001;
        vec3 specular = nominator / denominator;

        lo += (kd * albedo / PI + specular) * radiance * intensity * n_dot_l * compute_shadow(position);
    }

    o_color = vec4(lo, 1.0);
}
