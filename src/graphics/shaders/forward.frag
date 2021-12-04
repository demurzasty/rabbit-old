#version 460
#extension ARB_separate_shader_objects : enable

// Input 

layout (location = 0) in vec3 i_position;

// Output

layout (location = 0) out vec4 o_color;

void main() {
	o_color = vec4(1.0, 0.0, 0.0, 1.0);
}
