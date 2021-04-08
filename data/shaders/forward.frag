// Copyright (C) 2018-2021 Mariusz Dzikowski (thecyste@gmail.com)
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#define PI 3.14159265359

layout (location = 0) in vec3 var_position;
layout (location = 1) in vec2 var_texcoord;
layout (location = 2) in vec3 var_normal;

layout (std140, binding = 0) uniform Matrices {
    mat4 world;
    mat4 view;
    mat4 proj;
};

layout (std140, binding = 3) uniform ForwardData {
	vec3 u_diffuse; 
	float u_roughness;
	float u_metallic;
	vec3 u_light_dir;
	float u_light_intensity;
	vec3 u_light_color;
	vec3 u_camera_position;
};

layout (binding = 1) uniform samplerCube u_radiance;
layout (binding = 2) uniform samplerCube u_irradiance;
layout (binding = 3) uniform samplerCube u_prefilter;
layout (binding = 4) uniform sampler2D u_lut_map;

layout (binding = 5) uniform sampler2D u_diffuse_map;

layout (location = 0) out vec4 out_color;

float distribution_ggx(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float geometry_schlick_ggx(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness) {
	
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometry_schlick_ggx(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnel_schlick_roughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}   

void main() {
	vec3 albedo = u_diffuse;
	float roughness = u_roughness;
	float metallic = u_metallic;

	vec3 V = normalize(u_camera_position - var_position);
	vec3 N = normalize(var_normal);

	float n_dot_v = abs(dot(N, V)) + 1e-5;

	vec3 F0 = mix(vec3(0.04), albedo, metallic);

	vec3 Lo = vec3(0.0);
	{
		vec3 radiance = u_light_color;

		vec3 L = -normalize(u_light_dir);
		vec3 H = normalize(V + L);
		vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);

		vec3 kS = F;
		vec3 kD = (1.0 - kS) * (1.0 - metallic);

		float NDF = distribution_ggx(N, H, roughness);
		float G   = geometry_smith(N, V, L, roughness);

		float n_dot_l = max(dot(N, L), 0.0);
		
		vec3 nominator = NDF * G * F;
		float denominator = 4.0 * n_dot_v * n_dot_l + 0.001;
		vec3 specular = nominator / denominator;
		Lo += (kD * albedo / PI + specular) * radiance * u_light_intensity * n_dot_l;
	}

	vec3 kS = fresnel_schlick_roughness(max(dot(N, V), 0.0), F0, roughness);
	vec3 kD = (1.0 - kS) * (1.0 - metallic);

	vec3 irradiance = texture(u_irradiance, N).rgb;
	vec3 diff = irradiance * albedo;

	vec3 reflected = reflect(-V, N);

	vec3 prefiltered_color = textureLod(u_prefilter, reflected, roughness * 5.0).rgb;
	vec2 brdf = texture(u_lut_map, vec2(n_dot_v, roughness)).rg;
	vec3 spec = prefiltered_color * (kS * brdf.x + brdf.y);

	vec3 ambient = (kD * diff + spec);

	out_color = vec4(Lo + ambient, 1.0);
}