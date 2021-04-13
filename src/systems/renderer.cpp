#include <rabbit/systems/renderer.hpp>
#include <rabbit/graphics/builtin_shaders.hpp>
#include <rabbit/components/transform.hpp>
#include <rabbit/components/geometry.hpp>
#include <rabbit/components/camera.hpp>
#include <rabbit/math/mat4.hpp>

#include <iostream>

using namespace rb;

namespace {
    texture_cube_face texture_cube_faces[] = {
        texture_cube_face::positive_x,
        texture_cube_face::negative_x,
        texture_cube_face::positive_y,
        texture_cube_face::negative_y,
        texture_cube_face::positive_z,
        texture_cube_face::negative_z
    };
}

constexpr auto irradiance_map_size = 64;
constexpr auto prefilter_map_size = 128;
constexpr auto lut_map_size = 512;

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
        0, 1, 2,
        2, 3, 0,

        4, 5, 6,
        6, 7, 4,

        8, 9, 10,
        10, 11, 8,

        12, 13, 14,
        14, 15, 12,

        16, 17, 18,
        18, 19, 16,

        20, 21, 22,
        22, 23, 20
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

    texture_cube_desc texture_cube_desc;
    texture_cube_desc.size = { irradiance_map_size, irradiance_map_size };
    texture_cube_desc.filter = texture_filter::linear;
    texture_cube_desc.format = texture_format::rgba8;
    texture_cube_desc.is_render_target = true;
    texture_cube_desc.mipmaps = 1;
    _irradiance_map = graphics_device.make_texture(texture_cube_desc);

    buffer_desc.type = buffer_type::uniform;
    buffer_desc.data = nullptr;
    buffer_desc.stride = sizeof(irradiance_data);
    buffer_desc.size = sizeof(irradiance_data);
    buffer_desc.is_mutable = true;
    _irradiance_buffer = graphics_device.make_buffer(buffer_desc);

    texture_cube_desc.size = { prefilter_map_size, prefilter_map_size };
    texture_cube_desc.filter = texture_filter::linear;
    texture_cube_desc.format = texture_format::rgba8;
    texture_cube_desc.is_render_target = true;
    texture_cube_desc.mipmaps = true;
    texture_cube_desc.generate_mipmap = true;
    texture_cube_desc.mipmaps = 4;
    _prefilter_map = graphics_device.make_texture(texture_cube_desc);

    buffer_desc.type = buffer_type::uniform;
    buffer_desc.data = nullptr;
    buffer_desc.stride = sizeof(prefilter_data);
    buffer_desc.size = sizeof(prefilter_data);
    buffer_desc.is_mutable = true;
    _prefilter_buffer = graphics_device.make_buffer(buffer_desc);

    texture_desc texture_desc;
    texture_desc.size = { lut_map_size, lut_map_size };
    texture_desc.filter = texture_filter::linear;
    texture_desc.format = texture_format::rg8;
    texture_desc.is_render_target = true;
    _lut_map = graphics_device.make_texture(texture_desc);

    buffer_desc.type = buffer_type::uniform;
    buffer_desc.data = nullptr;
    buffer_desc.stride = sizeof(object_data);
    buffer_desc.size = sizeof(object_data);
    buffer_desc.is_mutable = true;
    _object_buffer = graphics_device.make_buffer(buffer_desc);

    buffer_desc.type = buffer_type::uniform;
    buffer_desc.data = nullptr;
    buffer_desc.stride = sizeof(light_data);
    buffer_desc.size = sizeof(light_data);
    buffer_desc.is_mutable = true;
    _light_buffer = graphics_device.make_buffer(buffer_desc);

    buffer_desc.type = buffer_type::uniform;
    buffer_desc.data = nullptr;
    buffer_desc.stride = sizeof(camera_data);
    buffer_desc.size = sizeof(camera_data);
    buffer_desc.is_mutable = true;
    _camera_buffer = graphics_device.make_buffer(buffer_desc);

    const auto backbuffer_size = graphics_device.backbuffer_size();

    graphics_device.set_backbuffer_size({ lut_map_size, lut_map_size });
    graphics_device.set_render_target(_lut_map);

    graphics_device.draw(_quad, _brdf);

    graphics_device.set_render_target(nullptr);

    _skybox_texture = asset_manager.load<texture_cube>("cubemaps/daylight.json");

    _generate_irradiance_map();
    _generate_prefilter_map();

    graphics_device.set_backbuffer_size(backbuffer_size);
}

void renderer::draw(registry& registry, graphics_device& graphics_device) {
    graphics_device.set_depth_test(true);
    graphics_device.clear(color::cornflower_blue());

    matrices_buffer_data matrices;
    matrices.projection = mat4f::perspective(45.0f, 1280.0f / 720.0, 0.1f, 100.0f);
    matrices.view = mat4f::inverse(mat4f::rotation_x(-pi<float>() * 0.125f) * mat4f::translation({ 0.0f, 1.0f, 5.0f }));
    matrices.world = mat4f::identity();

    graphics_device.bind_buffer_base(_matrices_buffer, 0);
    graphics_device.bind_buffer_base(_object_buffer, 3);
    graphics_device.bind_buffer_base(_light_buffer, 4);
    graphics_device.bind_buffer_base(_camera_buffer, 5);
    graphics_device.bind_texture(_irradiance_map, 2);
    graphics_device.bind_texture(_prefilter_map, 3);
    graphics_device.bind_texture(_lut_map, 4);

    light_data light_data;
    light_data.direction = vec3f::normalize({ -1.0f, -1.0f, -1.0f });
    light_data.color = { 1.0f, 1.0f, 1.0f };
    light_data.intensity = 1.0f;
    _light_buffer->update<renderer::light_data>({ &light_data, 1 });

    camera_data camera_data;

    auto backbuffer_size = graphics_device.backbuffer_size();
    registry.view<transform, camera>().each([&camera_data, &matrices, &backbuffer_size](transform& transform, camera& camera) {
        camera_data.position = transform.position;
        
        matrices.projection = mat4f::perspective(camera.fov, static_cast<float>(backbuffer_size.x) / backbuffer_size.y, 0.1f, 100.0f);
        matrices.view = mat4f::inverse(mat4f::translation(transform.position) * mat4f::rotation(transform.rotation));
    });

    _camera_buffer->update<renderer::camera_data>({ &camera_data, 1 });

#if 1
    registry.view<transform, geometry>().each([this, &graphics_device, &matrices](transform& transform, geometry& geometry) {
        object_data data;
        data.bitfield = 0;
        if (geometry.material) {
            data.diffuse = geometry.material->diffuse;
            data.metallic = geometry.material->metallic;
            data.roughness =  geometry.material->roughness;

            if (geometry.material->diffuse_map) {
                graphics_device.bind_texture(geometry.material->diffuse_map, 5);
                data.bitfield |= 1;
            }
        } else {
            data.diffuse = { 1.0f, 1.0f, 1.0f };
            data.metallic = 0.0f;
            data.roughness =  0.8f;
        }
        _object_buffer->update<object_data>({ &data, 1 });

        matrices.world = mat4f::translation(transform.position);
        _matrices_buffer->update<matrices_buffer_data>({ &matrices, 1 });

        graphics_device.draw(geometry.mesh, _forward);
    });
#endif

    graphics_device.set_blend_state(blend_state::opaque());
    graphics_device.set_depth_test(true);
    graphics_device.bind_texture(_skybox_texture, 2);

    matrices.view[12] = 0.0f;
    matrices.view[13] = 0.0f;
    matrices.view[14] = 0.0f;
    _matrices_buffer->update<matrices_buffer_data>({ &matrices, 1 });

    graphics_device.draw(_cube, _skybox);
}

void renderer::_generate_irradiance_map() {
    _graphics_device.bind_texture(_skybox_texture, 1);
    _graphics_device.bind_buffer_base(_irradiance_buffer, 1);
    _graphics_device.set_backbuffer_size({ irradiance_map_size, irradiance_map_size });

    irradiance_data data;
    for (auto face : texture_cube_faces) {
        data.cube_face = static_cast<int>(face);
        _irradiance_buffer->update<irradiance_data>({ &data, 1 });

        _graphics_device.set_render_target(_irradiance_map, face);

        _graphics_device.clear(rb::color::black());

        _graphics_device.draw(_quad, _irradiance);
    }
}

void renderer::_generate_prefilter_map() {
    _graphics_device.bind_texture(_skybox_texture, 1);
    _graphics_device.bind_buffer_base(_prefilter_buffer, 2);

    auto size = prefilter_map_size;
    auto mipmap = 0;

    prefilter_data data;
    while (mipmap < 4) {
        _graphics_device.set_backbuffer_size({ size, size });
        data.roughness = mipmap / 4.0f;

        for (auto face : texture_cube_faces) {
            data.cube_face = static_cast<int>(face);
            _prefilter_buffer->update<prefilter_data>({ &data, 1 });

            _graphics_device.set_render_target(_prefilter_map, face, mipmap);

            _graphics_device.clear(rb::color::black());

            _graphics_device.draw(_quad, _prefilter);
        }

        size /= 2;
        mipmap++;
    }
}
