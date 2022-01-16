#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#ifndef VULKAN
#define gl_InstanceIndex gl_BaseInstance
#endif

layout (location = 0) in vec3 v_position;
layout (location = 1) in flat uint v_id;

layout (location = 0) out uint o_id;

void main() {
	o_id = v_id;
}
