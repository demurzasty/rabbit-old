#version 450

layout (location = 0) in vec3 var_texcoord;

layout(set = 0, binding = 7) uniform sampler2D u_brdf_map;

layout(set = 1, binding = 0) uniform samplerCube u_radiance_map;
layout(set = 1, binding = 1) uniform samplerCube u_irradiance_map;
layout(set = 1, binding = 2) uniform samplerCube u_prefilter_map;

layout (location = 0) out vec4 o_color;
layout (location = 1) out vec4 o_bright;

void main() {
	o_color = vec4(texture(u_radiance_map, var_texcoord).rgb, 1.0);
	o_bright = vec4(0.0, 0.0, 0.0, 1.0);
}
