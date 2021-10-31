#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 v_texcoord;

layout (location = 0) out vec4 o_color;

layout (std140, set = 0, binding = 0) uniform camera_data {
    mat4 proj;
    mat4 view;
    mat4 u_inv_proj_view;
    vec3 u_camera_position;
};

layout (set = 0, binding = 1) uniform sampler2D u_brdf_map;

layout (set = 1, binding = 0) uniform sampler2D u_albedo_map;
layout (set = 1, binding = 1) uniform sampler2D u_normal_map;
layout (set = 1, binding = 2) uniform sampler2D u_emissive_map;
layout (set = 1, binding = 3) uniform sampler2D u_depth_map;

layout (set = 2, binding = 0) uniform samplerCube u_radiance_map;
layout (set = 2, binding = 1) uniform samplerCube u_irradiance_map;
layout (set = 2, binding = 2) uniform samplerCube u_prefilter_map;

vec3 fresnel_schlick_roughness(float cos_theta, vec3 f0, float roughness) {
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(1.0 - cos_theta, 5.0);
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

    vec3 ks = fresnel_schlick_roughness(max(dot(normal, v), 0.0), f0, roughness);
    vec3 kd = (1.0 - ks) * (1.0 - metallic);

    vec3 irradiance = texture(u_irradiance_map, normal).rgb;
    vec3 diff = irradiance * albedo;

    vec3 reflected = reflect(-v, normal);

    vec3 prefilter_color = textureLod(u_prefilter_map, reflected, roughness * 6.0).rgb;
    vec2 brdf = texture(u_brdf_map, vec2(n_dot_v, roughness)).rg;
    vec3 spec = prefilter_color * (ks * brdf.x + brdf.y);

    vec3 ambient = kd * diff + spec + texture(u_emissive_map, v_texcoord).rgb;

    o_color = vec4(ambient, 0.0);
}
