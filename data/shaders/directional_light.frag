#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define PI 3.14159265359
#define PCSS_BLOCKER_SEARCH_NUM_SAMPLES 8
#define PCF_NUM_SAMPLES 64

layout (location = 0) in vec2 v_texcoord;

layout (location = 0) out vec4 o_color;

layout (std140, set = 0, binding = 0) uniform camera_data {
    mat4 proj;
    mat4 view;
    mat4 u_inv_proj_view;
    vec3 u_camera_position;
};

layout(set = 0, binding = 1) uniform sampler2D u_brdf_map;
layout(set = 0, binding = 2) uniform sampler2D u_shadow_map;

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

const vec2 poisson_disk[64] = {
	vec2( -0.04117257, -0.1597612 ),
	vec2( 0.06731031, -0.4353096 ),
	vec2( -0.206701, -0.4089882 ),
	vec2( 0.1857469, -0.2327659 ),
	vec2( -0.2757695, -0.159873 ),
	vec2( -0.2301117, 0.1232693 ),
	vec2( 0.05028719, 0.1034883 ),
	vec2( 0.236303, 0.03379251 ),
	vec2( 0.1467563, 0.364028 ),
	vec2( 0.516759, 0.2052845 ),
	vec2( 0.2962668, 0.2430771 ),
	vec2( 0.3650614, -0.1689287 ),
	vec2( 0.5764466, -0.07092822 ),
	vec2( -0.5563748, -0.4662297 ),
	vec2( -0.3765517, -0.5552908 ),
	vec2( -0.4642121, -0.157941 ),
	vec2( -0.2322291, -0.7013807 ),
	vec2( -0.05415121, -0.6379291 ),
	vec2( -0.7140947, -0.6341782 ),
	vec2( -0.4819134, -0.7250231 ),
	vec2( -0.7627537, -0.3445934 ),
	vec2( -0.7032605, -0.13733 ),
	vec2( 0.8593938, 0.3171682 ),
	vec2( 0.5223953, 0.5575764 ),
	vec2( 0.7710021, 0.1543127 ),
	vec2( 0.6919019, 0.4536686 ),
	vec2( 0.3192437, 0.4512939 ),
	vec2( 0.1861187, 0.595188 ),
	vec2( 0.6516209, -0.3997115 ),
	vec2( 0.8065675, -0.1330092 ),
	vec2( 0.3163648, 0.7357415 ),
	vec2( 0.5485036, 0.8288581 ),
	vec2( -0.2023022, -0.9551743 ),
	vec2( 0.165668, -0.6428169 ),
	vec2( 0.2866438, -0.5012833 ),
	vec2( -0.5582264, 0.2904861 ),
	vec2( -0.2522391, 0.401359 ),
	vec2( -0.428396, 0.1072979 ),
	vec2( -0.06261792, 0.3012581 ),
	vec2( 0.08908027, -0.8632499 ),
	vec2( 0.9636437, 0.05915006 ),
	vec2( 0.8639213, -0.309005 ),
	vec2( -0.03422072, 0.6843638 ),
	vec2( -0.3734946, -0.8823979 ),
	vec2( -0.3939881, 0.6955767 ),
	vec2( -0.4499089, 0.4563405 ),
	vec2( 0.07500362, 0.9114207 ),
	vec2( -0.9658601, -0.1423837 ),
	vec2( -0.7199838, 0.4981934 ),
	vec2( -0.8982374, 0.2422346 ),
	vec2( -0.8048639, 0.01885651 ),
	vec2( -0.8975322, 0.4377489 ),
	vec2( -0.7135055, 0.1895568 ),
	vec2( 0.4507209, -0.3764598 ),
	vec2( -0.395958, -0.3309633 ),
	vec2( -0.6084799, 0.02532744 ),
	vec2( -0.2037191, 0.5817568 ),
	vec2( 0.4493394, -0.6441184 ),
	vec2( 0.3147424, -0.7852007 ),
	vec2( -0.5738106, 0.6372389 ),
	vec2( 0.5161195, -0.8321754 ),
	vec2( 0.6553722, -0.6201068 ),
	vec2( -0.2554315, 0.8326268 ),
	vec2( -0.5080366, 0.8539945 )
};

// pseudorandom number generator
float rand(vec2 co) {
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}
float rand(vec4 co) {
	float dot_product = dot(co, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
}

// parallel plane estimation
float penumbra_size(float z_receiver, float z_blocker) {
	return (z_receiver - z_blocker) / z_blocker * 12.0;
}

vec2 find_blocker(vec2 texcoord, float z_receiver) {
	ivec2 texture_size = textureSize(u_shadow_map, 0);
	vec2 texel = vec2(1.0 / texture_size.x, 1.0 / texture_size.y);

	float search_width = length(texel) * 20.0;
	float blockerSum = 0;
	float numBlockers = 0;
	
	for (int i = 0; i < PCSS_BLOCKER_SEARCH_NUM_SAMPLES; i++) {
		vec2 coord = texcoord + poisson_disk[i] * search_width;
		float smap = texture(u_shadow_map, coord).r;
		if (smap < z_receiver) {
			blockerSum += smap;
			numBlockers++;
		}
	}
	return vec2(blockerSum / numBlockers, numBlockers);
}

float pcf(vec2 texcoord, float z_receiver, float filter_radius) {
	ivec2 texture_size = textureSize(u_shadow_map, 0);
	vec2 texel = vec2(1.0 / texture_size.x, 1.0 / texture_size.y);

	float theta = rand(vec4(texcoord, gl_FragCoord.xy));
	mat2 rotation = mat2(vec2(cos(theta), sin(theta)), vec2(-sin(theta), cos(theta)));

	float shadow = 0.0;
	for (int i = 0; i < PCF_NUM_SAMPLES; ++i) {
		vec2 offset = rotation * poisson_disk[i] * texel * filter_radius;
		shadow += step(z_receiver, texture(u_shadow_map, texcoord + offset).r);
	}
	return shadow / PCF_NUM_SAMPLES;
}

float pcss(vec3 texcoord) {
	vec2 blockers = find_blocker(texcoord.xy, texcoord.z);
	return pcf(texcoord.xy, texcoord.z, 2.0 + penumbra_size(texcoord.z, blockers.x));
}

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
    vec4 shadow_position = u_light_proj_view * vec4(position, 1.0);
    shadow_position.xyz = shadow_position.xyz / shadow_position.w;

    vec2 shadow_coord = vec2(shadow_position.x, shadow_position.y) * 0.5 + 0.5;
    if (shadow_coord.x < 0.0 || shadow_coord.x > 1.0 ||
        shadow_coord.y < 0.0 || shadow_coord.y > 1.0 ||
        shadow_position.z < 0.0 || shadow_position.z > 1.0) {
        return 1.0;
    }

	float shadow = pcss(vec3(shadow_coord.xy, shadow_position.z));
	return sin(shadow * PI * 0.5);
}

void main() {
    vec4 albedo_data = texture(u_albedo_map, v_texcoord);
    vec4 normal_data = texture(u_normal_map, v_texcoord);
    vec4 emissive_data = texture(u_emissive_map, v_texcoord);

    vec3 position = extract_position(u_depth_map, v_texcoord);
    vec3 albedo = albedo_data.rgb;
    vec3 normal = normalize(normal_data.xyz * 2.0 - 1.0);
    float roughness = albedo_data.a;
    float metallic = normal_data.a;
	float ao = emissive_data.a;
    
    vec3 v = normalize(u_camera_position - position);
    float n_dot_v = abs(dot(normal, v)) + 1e-5;

    vec3 f0 = mix(vec3(0.04), albedo, metallic);

    float shadow = compute_shadow(position);

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

    vec3 lo = (kd * albedo / PI + specular) * radiance * intensity * n_dot_l * shadow;

    o_color = vec4(lo * ao, 0.0);
}
