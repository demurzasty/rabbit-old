#include <rabbit/renderer.hpp>
#include <rabbit/builtin_shaders.hpp>
#include <rabbit/transform.hpp>
#include <rabbit/geometry.hpp>
#include <rabbit/camera.hpp>
#include <rabbit/mat4.hpp>

using namespace rb;

struct matrices_buffer_data {
    mat4f world;
    mat4f view;
    mat4f projection;
};

renderer::renderer(graphics_device& graphics_device)
    : _graphics_device(graphics_device) {
    shader_desc desc;
    desc.vertex_bytecode = builtin_shaders::get(builtin_shader::forward_vert);
    desc.fragment_bytecode = builtin_shaders::get(builtin_shader::forward_frag);
    _forward = graphics_device.make_shader(desc);


    buffer_desc buffer_desc;
    buffer_desc.type = buffer_type::uniform;
    buffer_desc.size = sizeof(matrices_buffer_data);
    buffer_desc.stride = buffer_desc.size;
    buffer_desc.is_mutable = true;
    _matrices_buffer = graphics_device.make_buffer(buffer_desc);
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
        graphics_device.draw(topology::triangles, geometry.mesh->vertex_buffer(), _forward);
    });
}
