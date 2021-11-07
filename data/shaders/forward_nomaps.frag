#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define PI 3.14159265359

struct Light {
	vec3 dir_or_pos;
	float radius;
	vec3 color;
	int type;
};

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec2 v_texcoord;
layout (location = 2) in vec3 v_normal;

layout (std140, set = 0, binding = 0) uniform CameraData {
    mat4 proj;
    mat4 view;
    mat4 u_inv_proj_view;
    vec3 u_camera_position;
	mat4 u_light_proj_views[4];
};

layout (std140, set = 0, binding = 2) uniform LightListData {
    Light lights[16];
    mat4 light_proj_view;
    int light_count;
};

layout (std140, set = 1, binding = 0) uniform MaterialData {
    vec3 u_base_color;
    float u_roughness;
    float u_metallic;
};

layout(set = 0, binding = 1) uniform sampler2D u_brdf_map;
layout(set = 0, binding = 2) uniform sampler2DArray u_shadow_map;

layout(set = 2, binding = 0) uniform samplerCube u_radiance_map;
layout(set = 2, binding = 1) uniform samplerCube u_irradiance_map;
layout(set = 2, binding = 2) uniform samplerCube u_prefilter_map;

layout (location = 0) out vec4 out_color;

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

vec3 fresnel_schlick_roughness(float cos_theta, vec3 f0, float roughness) {
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(1.0 - cos_theta, 5.0);
}

mat3 cotangent_frame(vec3 n, vec3 p, vec2 uv) {
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx(p);
    vec3 dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    // solve the linear system
    vec3 dp2perp = cross(dp2, n);
    vec3 dp1perp = cross(n, dp1);
    vec3 t = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 b = dp2perp * duv1.y + dp1perp * duv2.y;

    // construct a scale-invariant frame
    float invmax = 1.0 / sqrt(max(dot(t, t), dot(b, b)));
    return mat3(t * invmax, b * invmax, n);
}

vec3 perturb(vec3 map, vec3 n, vec3 v, vec2 texcoord) {
  mat3 tbn = cotangent_frame(n, -v, texcoord);
  return normalize(tbn * map);
}

float compute_shadow() {
//    vec3 shadow_position = (v_shadow_coord.xyz / v_shadow_coord.w);
//
//    vec2 shadow_coord = vec2(shadow_position.x, shadow_position.y) * 0.5 + 0.5;
//    if (shadow_coord.x < 0.0 || shadow_coord.x > 1.0 ||
//        shadow_coord.y < 0.0 || shadow_coord.y > 1.0) {
//        return 1.0;
//    }
//    
    return 1.0;
    //return step(shadow_position.z, texture(u_shadow_map, shadow_coord).x);
}

void main() {
    vec4 albedo = vec4(u_base_color, 1.0);
    vec3 normal = v_normal;
    float roughness = u_roughness;
    float metallic = u_metallic;
    vec3 emissive = vec3(0.0);
    float ao = 1.0;

    vec3 v = normalize(u_camera_position - v_position);
    vec3 n = normalize(normal);

    float n_dot_v = abs(dot(n, v)) + 1e-5;

    vec3 f0 = mix(vec3(0.04), albedo.rgb, metallic);

    vec3 lo = vec3(0.0);

    float shadow = compute_shadow(); // texture_proj(v_shadow_coord / v_shadow_coord.w, vec2(0.0));

    // TODO: Iterate through nearest lights.
//    for (int i = 0; i < light_count; ++i)
//    {
//        // TODO: Light color and intensity.
//        vec3 radiance = lights[i].color.xyz;
//        float intensity = 1.0;
//
//        vec3 l = vec3(0.0);
//        
//        if (lights[i].type == 0) {
//            l = -normalize(lights[i].dir_or_pos.xyz);
//        } else if (lights[i].type == 1) {
//            l = normalize(lights[i].dir_or_pos.xyz - v_position);
//            float distance = length(lights[i].dir_or_pos.xyz - v_position);
//            float attenuation = pow(clamp(1.0 - pow(distance / lights[i].radius, 4.0), 0.0, 1.0), 2.0) / (distance * distance + 1.0);
//            radiance *= attenuation;
//        }
//
//        vec3 h = normalize(v + l);
//        vec3 f = fresnel_schlick(max(dot(h, v), 0.0), f0);
//
//        vec3 ks = f;
//        vec3 kd = (1.0 - ks) * (1.0 - metallic);
//
//        float ndf = distribution_ggx(n, h, roughness);
//        float g = geometry_smith(n, v, l, roughness);
//
//        float n_dot_l = max(dot(n, l), 0.0);
//
//        vec3 nominator = ndf * g * f;
//        float denominator = 4.0 * n_dot_v * n_dot_l + 0.001;
//        vec3 specular = nominator / denominator;
//
//        lo += (kd * albedo.rgb / PI + specular) * radiance * intensity * n_dot_l * shadow;
//    }

    vec3 ks = fresnel_schlick_roughness(max(dot(n, v), 0.0), f0, roughness);
    vec3 kd = (1.0 - ks) * (1.0 - metallic);

    vec3 irradiance = texture(u_irradiance_map, n).rgb;
    vec3 diff = irradiance * albedo.rgb;

    vec3 reflected = reflect(-v, n);

    vec3 prefilter_color = textureLod(u_prefilter_map, reflected, roughness * 6.0).rgb;
    vec2 brdf = texture(u_brdf_map, vec2(n_dot_v, roughness)).rg;
    vec3 spec = prefilter_color * (ks * brdf.x + brdf.y);

    vec3 ambient = kd * diff + spec + emissive;

    out_color = vec4(ambient + lo, 1.0);
}
