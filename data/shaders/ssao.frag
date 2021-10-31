#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define SSAO_SAMPLES 64

layout (location = 0) in vec2 v_texcoord;

layout (location = 0) out vec4 o_color;

layout (std140, set = 0, binding = 0) uniform camera_data {
    mat4 u_proj;
    mat4 u_view;
    mat4 u_inv_proj_view;
    vec3 u_camera_position;
};

layout (set = 1, binding = 0) uniform sampler2D u_albedo_map;
layout (set = 1, binding = 1) uniform sampler2D u_normal_map;
layout (set = 1, binding = 2) uniform sampler2D u_emissive_map;
layout (set = 1, binding = 3) uniform sampler2D u_depth_map;

layout (set = 2, binding = 0) uniform ssao_data {
    vec3 u_samples[SSAO_SAMPLES];
};
layout (set = 2, binding = 1) uniform sampler2D u_noise_map;

layout (set = 3, binding = 0) uniform sampler2D u_postprocess_map;

const int kernel_size = SSAO_SAMPLES;
const float radius = 0.25;
const float bias = 0.025;

const vec2 noise_scale = vec2(1280.0 / 4.0, 720.0 / 4.0);

vec3 extract_position(float depth, vec2 texcoord) {
    vec4 position;
	position.x = texcoord.x * 2.0 - 1.0;
    position.y = -(texcoord.y * 2.0 - 1.0);
	position.z = depth;
	position.w = 1.0;
	position = u_inv_proj_view * position;
	return position.xyz / position.w;
}

float linearize_depth(float depth, float near, float far) {
// return depth;
    return near * far / (far + depth * (near - far));
}

void main() {
    float depth = texture(u_depth_map, v_texcoord).r;
    vec3 position = (u_view * vec4(extract_position(depth, v_texcoord), 1.0)).xyz;
    vec3 normal = (u_view * vec4(normalize(texture(u_normal_map, v_texcoord).xyz * 2.0 - 1.0), 0.0)).xyz;
    vec3 random_vec = normalize(vec3(texture(u_noise_map, v_texcoord * noise_scale).xy, 0.0));

    vec3 tangent = normalize(random_vec - normal * dot(random_vec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);

    //float linearized_depth = linearize_depth(depth, 0.1, 100.0);
    // float linearized_depth = (u_proj * vec4(position, 1.0)).z;
    float linearized_depth = position.z;

    float occlusion = 0.0;
    for (int i = 0; i < kernel_size; ++i) {
        vec3 sample_pos = tbn * u_samples[i];
        sample_pos = position + sample_pos * radius;

        vec4 offset = vec4(sample_pos, 1.0);
        offset = u_proj * offset;
        offset.xyz /= offset.w;
        offset.y = -offset.y;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        //vec4 pos2 = u_proj * vec4(extract_position(texture(u_depth_map, offset.xy).r, offset.xy), 1.0);
        //float sample_depth = pos2.z;
        float sample_depth = (u_view * vec4(extract_position(texture(u_depth_map, offset.xy).r, offset.xy), 1.0)).z;
        //float sample_depth =  linearize_depth(texture(u_depth_map, offset.xy).r, 0.1, 100.0);
        vec3 sample_normal = (u_view * vec4(normalize(texture(u_normal_map, offset.xy).xyz * 2.0 - 1.0), 0.0)).xyz;

        float factor = distance(position, sample_pos) / radius;

        float range_check = smoothstep(0.0, 1.0, radius / abs(linearized_depth - sample_depth));
        float normal_check = max(1.0 - dot(sample_normal, normal), 0.0);
        occlusion += step(linearized_depth + bias, sample_depth) * range_check * normal_check * normal_check;
        //occlusion += range_check;
    }
    occlusion = (occlusion / kernel_size);

    o_color = vec4(vec3(1.0 - occlusion) * texture(u_postprocess_map, v_texcoord).rgb, 1.0);
    // o_color = vec4(vec3(1.0 - occlusion), 1.0);
    // o_color = vec4(vec3(0.0), occlusion);
    //o_color = vec4(position, 1.0);
    // o_color = vec4(1.0, 0.0, 0.0, 1.0);
}
