// Copyright (C) 2018-2021 Mariusz Dzikowski (thecyste@gmail.com)
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#define PI 3.14159265359

#define DIFFUSE_MAP_BIT 1
#define NORMAL_MAP_BIT 2
#define ROUGHNESS_MAP_BIT 4
#define METALLIC_MAP_BIT 8
#define EMISSIVE_MAP_BIT 16

layout (location = 0) in vec3 var_position;
layout (location = 1) in vec2 var_texcoord;
layout (location = 2) in vec3 var_normal;

layout (std140, binding = 0) uniform Matrices {
    mat4 world;
    mat4 view;
    mat4 proj;
};

layout (std140, binding = 3) uniform ObjectData {
	vec3 u_diffuse; 
	float u_roughness;
	float u_metallic;
	int u_bitfield;
};

layout (std140, binding = 4) uniform LightData {
	vec3 u_light_dir;
	float u_light_intensity;
	vec3 u_light_color;
};

layout (std140, binding = 5) uniform CameraData {
	vec3 u_camera_position;
};

layout (binding = 1) uniform samplerCube u_radiance;
layout (binding = 2) uniform samplerCube u_irradiance;
layout (binding = 3) uniform samplerCube u_prefilter;
layout (binding = 4) uniform sampler2D u_lut;

layout (binding = 5) uniform sampler2D u_diffuse_map;
layout (binding = 6) uniform sampler2D u_normal_map;
layout (binding = 7) uniform sampler2D u_roughness_map;
layout (binding = 8) uniform sampler2D u_metallic_map;
layout (binding = 9) uniform sampler2D u_emissive_map;

layout (location = 0) out vec4 out_color;

float distribution_ggx(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float n_dot_h = max(dot(N, H), 0.0);
    float n_dot_h2 = n_dot_h * n_dot_h;

    float nom   = a2;
    float denom = (n_dot_h2 * (a2 - 1.0) + 1.0);
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

void main() {
	vec3 albedo = u_diffuse;
	if ((u_bitfield & DIFFUSE_MAP_BIT) != 0) {
		albedo *= texture(u_diffuse_map, var_texcoord).rgb;
	}

	float roughness = u_roughness;
	if ((u_bitfield & ROUGHNESS_MAP_BIT) != 0) {
		roughness *= texture(u_roughness_map, var_texcoord).r;
	}

	float metallic = u_metallic;
	if ((u_bitfield & METALLIC_MAP_BIT) != 0) {
		metallic *= texture(u_metallic_map, var_texcoord).r;
	}

	vec3 V = normalize(u_camera_position - var_position);

	vec3 N = normalize(var_normal);
	if ((u_bitfield & NORMAL_MAP_BIT) != 0) {
		vec3 normal = texture(u_normal_map, var_texcoord).rgb * 2.0 - 1.0;
		N = perturb(normal, N, V, var_texcoord);
	}

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

	vec2 brdf = texture(u_lut, vec2(n_dot_v, roughness)).rg;
	vec3 spec = prefiltered_color * (kS * brdf.x + brdf.y);

	vec3 ambient = (kD * diff + spec);

	if ((u_bitfield & EMISSIVE_MAP_BIT) != 0) {
		Lo += texture(u_emissive_map, var_texcoord).rgb;
	}

	out_color = vec4(Lo + ambient, 1.0);
}