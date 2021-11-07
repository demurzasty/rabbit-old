#version 450

layout (constant_id = 0) const int ALBEDO_MAP = 0;
layout (constant_id = 1) const int NORMAL_MAP = 1;
layout (constant_id = 2) const int ROUGHNESS_MAP = 2;
layout (constant_id = 3) const int METALLIC_MAP = 3;
layout (constant_id = 4) const int EMISSIVE_MAP = 4;
layout (constant_id = 5) const int AMBIENT_MAP = 5;
layout (constant_id = 6) const int MAX_MAPS = 6;

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

layout(set = 1, binding = 1) uniform sampler2D u_maps[MAX_MAPS];

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
    if (ALBEDO_MAP > -1) {
        albedo *= texture(u_maps[ALBEDO_MAP], v_texcoord);
        if (albedo.a < 0.5) {
            discard;
        }
    }

    vec3 normal = v_normal;
    if (NORMAL_MAP > -1) {
        normal = perturb(texture(u_maps[NORMAL_MAP], v_texcoord).rgb * 2.0 - 1.0, normalize(normal), normalize(u_camera_position - v_position), v_texcoord);
    }

    float roughness = u_roughness;
    if (ROUGHNESS_MAP > -1) {
        roughness *= texture(u_maps[ROUGHNESS_MAP], v_texcoord).g;
    }
   
    float metallic = u_metallic;
    if (METALLIC_MAP > -1) {
        metallic *= texture(u_maps[METALLIC_MAP], v_texcoord).b;
    }
    
    vec3 emissive = vec3(0.0);
    if (EMISSIVE_MAP > -1) {
        emissive = texture(u_maps[EMISSIVE_MAP], v_texcoord).rgb;
    }

    float ao = 1.0;
    if (AMBIENT_MAP > -1) {
        ao = texture(u_maps[AMBIENT_MAP], v_texcoord).r;
    }

    o_albedo = vec4(albedo.rgb, roughness);
    o_normal = vec4(normal * 0.5 + 0.5, metallic);
    o_emissive = vec4(emissive, ao);
}
