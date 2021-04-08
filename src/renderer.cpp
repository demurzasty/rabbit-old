#include <rabbit/renderer.hpp>
#include <rabbit/builtin_shaders.hpp>
#include <rabbit/transform.hpp>
#include <rabbit/geometry.hpp>
#include <rabbit/camera.hpp>
#include <rabbit/mat4.hpp>

#include <iostream>

using namespace rb;

struct matrices_buffer_data {
    mat4f world;
    mat4f view;
    mat4f projection;
};

renderer::renderer(graphics_device& graphics_device, asset_manager& asset_manager)
    : _graphics_device(graphics_device) {
    try {
        shader_desc desc;
        desc.vertex_desc = {
            { vertex_attribute::position, vertex_format::vec3f() },
            { vertex_attribute::texcoord, vertex_format::vec2f() },
            { vertex_attribute::normal, vertex_format::vec3f() }
        };
        desc.vertex_bytecode = builtin_shaders::get(builtin_shader::forward_vert);
        desc.fragment_bytecode = builtin_shaders::get(builtin_shader::forward_frag);
        _forward = graphics_device.make_shader(desc);

        desc.vertex_desc = { { vertex_attribute::position, vertex_format::vec2f() } };
        desc.vertex_bytecode = builtin_shaders::get(builtin_shader::irradiance_vert);
        desc.fragment_bytecode = builtin_shaders::get(builtin_shader::irradiance_frag);
        _irradiance = graphics_device.make_shader(desc);

        desc.vertex_desc = { { vertex_attribute::position, vertex_format::vec2f() } };
        desc.vertex_bytecode = builtin_shaders::get(builtin_shader::brdf_vert);
        desc.fragment_bytecode = builtin_shaders::get(builtin_shader::brdf_frag);
        _brdf = graphics_device.make_shader(desc);

        desc.vertex_desc = { { vertex_attribute::position, vertex_format::vec2f() } };
        desc.vertex_bytecode = builtin_shaders::get(builtin_shader::prefilter_vert);
        desc.fragment_bytecode = builtin_shaders::get(builtin_shader::prefilter_frag);
        _prefilter = graphics_device.make_shader(desc);

        desc.vertex_desc = {
            { vertex_attribute::position, vertex_format::vec3f() },
            { vertex_attribute::texcoord, vertex_format::vec2f() },
            { vertex_attribute::normal, vertex_format::vec3f() }
        };
        desc.vertex_bytecode = builtin_shaders::get(builtin_shader::skybox_vert);
        desc.fragment_bytecode = builtin_shaders::get(builtin_shader::skybox_frag);
        _skybox = graphics_device.make_shader(desc);
    } catch (const exception& exception) {
        std::cerr << exception.what() << std::endl;
    }

    buffer_desc buffer_desc;
    buffer_desc.type = buffer_type::uniform;
    buffer_desc.size = sizeof(matrices_buffer_data);
    buffer_desc.stride = buffer_desc.size;
    buffer_desc.is_mutable = true;
    _matrices_buffer = graphics_device.make_buffer(buffer_desc);

    vec3f cube_vertices[24] = {
		{ -1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, -1.0f, 1.0f },
		{ -1.0f, -1.0f, 1.0f },

		{ -1.0f, 1.0f, -1.0f },
		{ -1.0f, 1.0f, 1.0f },
		{ -1.0f, -1.0f, 1.0f },
		{ -1.0f, -1.0f, -1.0f },

		{ 1.0f, 1.0f, -1.0f },
		{ -1.0f, 1.0f, -1.0f },
		{ -1.0f, -1.0f, -1.0f },
		{ 1.0f, -1.0f, -1.0f },

		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, -1.0f },
		{ 1.0f, -1.0f, -1.0f },
		{ 1.0f, -1.0f, 1.0f },

		{ -1.0f, 1.0f, -1.0f },
		{ 1.0f, 1.0f, -1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ -1.0f, 1.0f, 1.0f },

		{ -1.0f, -1.0f, 1.0f },
		{ 1.0f, -1.0f, 1.0f },
		{ 1.0f, -1.0f, -1.0f },
		{ -1.0f, -1.0f, -1.0f },
    };

    std::uint32_t cube_indices[36] = {
        0, 2, 1,
        2, 0, 3,

        4, 6, 5,
        6, 4, 7,

        8, 10, 9,
        10, 8, 11,

        12, 14, 13,
        14, 12, 15,

        16, 18, 17,
        18, 16, 19,

        20, 22, 21,
        22, 20, 23
    };

    buffer_desc.type = buffer_type::vertex;
    buffer_desc.data = cube_vertices;
    buffer_desc.stride = sizeof(vec3f);
    buffer_desc.size = sizeof(cube_vertices);
    buffer_desc.is_mutable = false;
    auto cube_vertex_buffer = graphics_device.make_buffer(buffer_desc);

    buffer_desc.type = buffer_type::index;
    buffer_desc.data = cube_indices;
    buffer_desc.stride = sizeof(std::uint32_t);
    buffer_desc.size = sizeof(cube_indices);
    buffer_desc.is_mutable = false;
    auto cube_index_buffer = graphics_device.make_buffer(buffer_desc);

    mesh_desc mesh_desc;
    mesh_desc.topology = topology::triangles;
    mesh_desc.vertex_desc = { { vertex_attribute::position, vertex_format::vec3f() } };
    mesh_desc.vertex_buffer = cube_vertex_buffer;
    mesh_desc.index_buffer = cube_index_buffer;
    _cube = graphics_device.make_mesh(mesh_desc);

    vec2f quad_vertices[] = {
        { -1.0f, 1.0f },
        { -1.0f, -1.0f },
        { 1.0f, -1.0f },

        { 1.0f, -1.0f },
        { 1.0f, 1.0f },
        { -1.0f, 1.0f }
    };

    buffer_desc.type = buffer_type::vertex;
    buffer_desc.data = quad_vertices;
    buffer_desc.stride = sizeof(vec2f);
    buffer_desc.size = sizeof(quad_vertices);
    buffer_desc.is_mutable = false;
    auto quad_vertex_buffer = graphics_device.make_buffer(buffer_desc);
    
    mesh_desc.topology = topology::triangles;
    mesh_desc.vertex_desc = { { vertex_attribute::position, vertex_format::vec2f() } };
    mesh_desc.vertex_buffer = cube_vertex_buffer;
    mesh_desc.index_buffer = nullptr;
    _quad = graphics_device.make_mesh(mesh_desc);

    _skybox_texture = asset_manager.load<texture_cube>("cubemaps/magic_hour.json");
}

void renderer::draw(registry& registry, graphics_device& graphics_device) {
    graphics_device.set_depth_test(true);
    graphics_device.clear(color::cornflower_blue());

    matrices_buffer_data matrices;
    matrices.projection = mat4f::perspective(45.0f, 1280.0f / 720.0, 0.1f, 100.0f);
    matrices.view = mat4f::inverse(mat4f::rotation_x(-pi<float>() * 0.125f) * mat4f::translation({ 0.0f, 0.0f, 10.0f }));
    matrices.world = mat4f::identity();
    _matrices_buffer->update<matrices_buffer_data>({ &matrices, 1 });

    graphics_device.bind_buffer_base(_matrices_buffer, 0);

    registry.view<transform, geometry>().each([this, &graphics_device](transform& transform, geometry& geometry) {
        graphics_device.draw(geometry.mesh, _forward);
    });

    graphics_device.set_blend_state(blend_state::opaque());
    graphics_device.set_depth_test(true);
    graphics_device.bind_texture(_skybox_texture, 2);

    matrices.view[12] = 0.0f;
    matrices.view[13] = 0.0f;
    matrices.view[14] = 0.0f;
    _matrices_buffer->update<matrices_buffer_data>({ &matrices, 1 });

    graphics_device.draw(_cube, _skybox);
}
