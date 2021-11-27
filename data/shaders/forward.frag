#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define PI 3.14159265359
#define PCSS_BLOCKER_SEARCH_NUM_SAMPLES 4
#define PCF_NUM_SAMPLES 64

#ifndef MAX_SHADOW_MAP_CASCADES
#define MAX_SHADOW_MAP_CASCADES 4
#endif

struct light {
	vec4 position_or_direction; // .a < 0.5 ? point_light : directional_light
	vec4 color; // .a = radius 
};

struct visible_index {
	int index;
};

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec2 v_texcoord;
layout (location = 2) in vec3 v_normal;

layout (std140, set = 0, binding = 0) uniform camera_data {
    mat4 proj;
    mat4 view;
    mat4 inv_proj_view;
    vec3 position;
	mat4 light_proj_views[4];
} u_camera;

layout(set = 0, binding = 1) uniform sampler2D u_brdf_map;

#ifdef SHADOW_MAP
layout(set = 0, binding = 2) uniform sampler2DArray u_shadow_map;
#endif

layout (std140, set = 1, binding = 0) uniform material_data {
    vec4 base_color;
    float roughness;
    float metallic;
    float occlusion_strength;
} u_material;

#ifdef ALBEDO_MAP 
layout (set = 1, binding = 1) uniform sampler2D u_albedo_map;
#endif 

#ifdef NORMAL_MAP 
layout (set = 1, binding = 2) uniform sampler2D u_normal_map;
#endif

#ifdef ROUGHNESS_MAP 
layout (set = 1, binding = 3) uniform sampler2D u_roughness_map;
#endif

#ifdef METALLIC_MAP 
layout (set = 1, binding = 4) uniform sampler2D u_metallic_map;
#endif

#ifdef EMISSIVE_MAP 
layout (set = 1, binding = 5) uniform sampler2D u_emissive_map;
#endif

#ifdef AMBIENT_MAP 
layout (set = 1, binding = 6) uniform sampler2D u_ambient_map;
#endif

layout(set = 2, binding = 0) uniform samplerCube u_radiance_map;
layout(set = 2, binding = 1) uniform samplerCube u_irradiance_map;
layout(set = 2, binding = 2) uniform samplerCube u_prefilter_map;

// Shader storage buffer objects
layout(std430, set = 3, binding = 0) readonly buffer light_buffer {
	light data[];
} u_light_buffer;

layout(std430, set = 3, binding = 1) readonly buffer visible_light_indices_buffer {
	visible_index data[];
} u_visible_light_indices_buffer;

layout(std140, set = 3, binding = 2) uniform culling_data {
	uint light_count;
    uint number_of_tiles_x;
} u_culling_data;

layout (location = 0) out vec4 out_color;

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
	return max((z_receiver - z_blocker) / z_blocker * 12.0, 0.0);
}

#ifdef SHADOW_MAP
vec2 find_blocker(vec2 texcoord, float z_receiver, float cascade) {
	ivec2 texture_size = textureSize(u_shadow_map, 0).xy;
	vec2 texel = vec2(1.0 / texture_size.x, 1.0 / texture_size.y);

	float search_width = length(texel) * 20.0 / (cascade + 1);
	float blockerSum = 0;
	float numBlockers = 0;
	
	for (int i = 0; i < PCSS_BLOCKER_SEARCH_NUM_SAMPLES; i++) {
		vec2 coord = texcoord + poisson_disk[i] * search_width;
		float smap = texture(u_shadow_map, vec3(coord, cascade)).r;
		if (smap < z_receiver) {
			blockerSum += smap;
			numBlockers++;
		}
	}
	return vec2(blockerSum / numBlockers, numBlockers);
}

float pcf(vec2 texcoord, float z_receiver, float filter_radius, float cascade) {
	ivec2 texture_size = textureSize(u_shadow_map, 0).xy;
	vec2 texel = vec2(1.0 / texture_size.x, 1.0 / texture_size.y);

	float theta = rand(vec4(texcoord, gl_FragCoord.xy));
	mat2 rotation = mat2(vec2(cos(theta), sin(theta)), vec2(-sin(theta), cos(theta)));

	float shadow = 0.0;
	for (int i = 0; i < PCF_NUM_SAMPLES; ++i) {
		vec2 offset = rotation * poisson_disk[i] * texel * filter_radius;
		shadow += step(z_receiver, texture(u_shadow_map, vec3(texcoord + offset, cascade)).r);
	}
	return shadow / PCF_NUM_SAMPLES;
}

float pcss(vec3 texcoord, float cascade) {
	vec2 blockers = find_blocker(texcoord.xy, texcoord.z, cascade);
	return pcf(texcoord.xy, texcoord.z, 2.0 + penumbra_size(texcoord.z, blockers.x), cascade);
}
#endif

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
#ifdef SHADOW_MAP
	for (int i = 0; i < MAX_SHADOW_MAP_CASCADES; ++i) {
		vec4 shadow_coord = u_camera.light_proj_views[i] * vec4(v_position, 1.0);
		shadow_coord.xyz = shadow_coord.xyz / shadow_coord.w;
		shadow_coord.y = -shadow_coord.y;
		shadow_coord.xy = shadow_coord.xy * 0.5 + 0.5;

		if (shadow_coord.x >= 0.0 && shadow_coord.x <= 1.0 &&
			shadow_coord.y >= 0.0 && shadow_coord.y <= 1.0 &&
			shadow_coord.z >= -1.0 && shadow_coord.z <= 1.0) {
			return pcf(shadow_coord.xy, shadow_coord.z - 0.002, 3.0, i);
		} 
	}
#endif
	return 1.0;
}

void main() {
	ivec2 location = ivec2(gl_FragCoord.xy);
	ivec2 tile_id = location / ivec2(16, 16);
	uint index = tile_id.y * u_culling_data.number_of_tiles_x + tile_id.x;

    vec4 albedo = u_material.base_color;
#ifdef ALBEDO_MAP
    albedo *= texture(u_albedo_map, v_texcoord);
#ifndef TRANSLUCENT
	// TODO: Customizable cutoff.
	if (albedo.a < 0.5) {
		discard;
	}
#endif
#endif

    vec3 normal = v_normal;
#ifdef NORMAL_MAP
   normal = perturb(texture(u_normal_map, v_texcoord).rgb * 2.0 - 1.0, normalize(normal), normalize(u_camera.position - v_position), v_texcoord);
#endif

    float roughness = u_material.roughness;
#ifdef ROUGHNESS_MAP
    roughness *= texture(u_roughness_map, v_texcoord).g;
#endif
   
    float metallic = u_material.metallic;
#ifdef METALLIC_MAP
    metallic *= texture(u_metallic_map, v_texcoord).b;
#endif
    
    vec3 emissive = vec3(0.0);
#ifdef EMISSIVE_MAP
    emissive = texture(u_emissive_map, v_texcoord).rgb;
#endif

    float ao = 1.0;
#ifdef AMBIENT_MAP
    ao = texture(u_ambient_map, v_texcoord).r;
#endif

    vec3 v = normalize(u_camera.position - v_position);
    vec3 n = normalize(normal);

    float n_dot_v = abs(dot(n, v)) + 1e-5;

    vec3 f0 = mix(vec3(0.04), albedo.rgb, metallic);

    vec3 lo = vec3(0.0);

	uint offset = index * 1024;
	for (uint i = 0; i < 1024 && u_visible_light_indices_buffer.data[offset + i].index != -1; ++i) {
		uint light_index = u_visible_light_indices_buffer.data[offset + i].index;
		light light = u_light_buffer.data[light_index];

        vec3 radiance = light.color.xyz;

        vec3 l = vec3(0.0);
        float shadow = 1.0;

        if (light.position_or_direction.w > 0.5) {
            l = -normalize(light.position_or_direction.xyz);
#ifdef SHADOW_MAP
			if (light.color.w > 0.5) {
				shadow = compute_shadow();
			}
#endif
        } else {
			vec3 diff = light.position_or_direction.xyz - v_position;
            float dist = length(diff);
            l = diff / dist;
			float radius = light.color.w;
            float attenuation = pow(clamp(1.0 - pow(dist / radius, 4.0), 0.0, 1.0), 2.0) / (dist * dist + 1.0);
            radiance *= attenuation;
        }

        vec3 h = normalize(v + l);
        vec3 f = fresnel_schlick(max(dot(h, v), 0.0), f0);

        vec3 ks = f;
        vec3 kd = (1.0 - ks) * (1.0 - metallic);

        float ndf = distribution_ggx(n, h, roughness);
        float g = geometry_smith(n, v, l, roughness);

        float n_dot_l = max(dot(n, l), 0.0);

        vec3 nominator = ndf * g * f;
        float denominator = 4.0 * n_dot_v * n_dot_l + 0.001;
        vec3 specular = nominator / denominator;

        lo += (kd * albedo.rgb / PI + specular) * radiance * n_dot_l * shadow;
	}

    vec3 ks = fresnel_schlick_roughness(max(dot(n, v), 0.0), f0, roughness);
    vec3 kd = (1.0 - ks) * (1.0 - metallic);

    vec3 irradiance = texture(u_irradiance_map, n).rgb;
    vec3 diff = irradiance * albedo.rgb;

    vec3 reflected = reflect(-v, n);

    vec3 prefilter_color = textureLod(u_prefilter_map, reflected, roughness * 6.0).rgb;
    vec2 brdf = texture(u_brdf_map, vec2(n_dot_v, roughness)).rg;
    vec3 spec = prefilter_color * (ks * brdf.x + brdf.y);

    vec3 ambient = (kd * diff + spec);
	
#ifdef AMBIENT_MAP
	ambient = mix(ambient, ambient * ao, u_material.occlusion_strength);
#endif

    out_color = vec4(ambient + lo + emissive, albedo.a);
}
