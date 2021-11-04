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

layout(set = 1, binding = 1) uniform sampler2D u_albedo_map;
layout(set = 1, binding = 2) uniform sampler2D u_normal_map;
layout(set = 1, binding = 3) uniform sampler2D u_roughness_map;
layout(set = 1, binding = 4) uniform sampler2D u_metallic_map;
layout(set = 1, binding = 5) uniform sampler2D u_emissive_map;
layout(set = 1, binding = 6) uniform sampler2D u_ambient_map;

layout (constant_id = 0) const int USE_ALBEDO_MAP = 0;
layout (constant_id = 1) const int USE_NORMAL_MAP = 0;
layout (constant_id = 2) const int USE_ROUGHNESS_MAP = 0;
layout (constant_id = 3) const int USE_METALLIC_MAP = 0;
layout (constant_id = 4) const int USE_EMISSIVE_MAP = 0;
layout (constant_id = 5) const int USE_AMBIENT_MAP = 0;

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
    mat3 tbn = cotangent_frame(n, v, texcoord);
    return normalize(tbn * map);
}

void main() {
    vec4 albedo = vec4(u_base_color, 1.0);
    if (USE_ALBEDO_MAP != 0) {
        albedo *= texture(u_albedo_map, v_texcoord);
        if (albedo.a < 0.5) {
            discard;
        }
    }

    vec3 normal = v_normal;
    if (USE_NORMAL_MAP != 0) {
        normal = perturb(texture(u_normal_map, v_texcoord).rgb * 2.0 - 1.0, normalize(normal), normalize(u_camera_position - v_position), v_texcoord);
    }

    float roughness = u_roughness;
    if (USE_ROUGHNESS_MAP != 0) {
        roughness *= texture(u_roughness_map, v_texcoord).r;
    }
   
    float metallic = u_metallic;
    if (USE_METALLIC_MAP != 0) {
        metallic *= texture(u_metallic_map, v_texcoord).r;
    }
    
    vec3 emissive = vec3(0.0);
    if (USE_EMISSIVE_MAP != 0) {
        emissive = texture(u_emissive_map, v_texcoord).rgb;
    }


    float ao = 1.0;
    if (USE_AMBIENT_MAP != 0) {
        ao = texture(u_ambient_map, v_texcoord).r;
    }

    o_albedo = vec4(albedo.rgb, roughness);
    o_normal = vec4(normal * 0.5 + 0.5, metallic);
    o_emissive = vec4(emissive, ao);
}
