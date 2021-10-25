#version 450

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texcoord;
layout (location = 2) in vec3 in_normal;

layout (std140, push_constant) uniform ShadowData {
    mat4 proj_view_world;
};

out gl_PerVertex {
    vec4 gl_Position;   
};

void main() {
	gl_Position =  proj_view_world * vec4(in_position, 1.0);
}
