#version 450

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texcoord;
layout (location = 2) in vec3 in_normal;

layout (binding = 0) uniform Matrices {
    mat4 proj;
    mat4 view;
    mat4 world;
};

void main() {
    gl_Position = world * vec4(in_position, 1.0);
}

// void main() {
//     const vec3 positions[3] = vec3[3](
// 		vec3(1.0, 1.0, 0.0),
// 		vec3(-1.0, 1.0, 0.0),
// 		vec3(0.0,-1.0, 0.0)
//     );

//     gl_Position = vec4(positions[gl_VertexIndex], 1.0);
// }
