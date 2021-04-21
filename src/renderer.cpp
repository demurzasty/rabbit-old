#include <rabbit/renderer.hpp>
#include <rabbit/transform.hpp>
#include <rabbit/geometry.hpp>
#include <rabbit/camera.hpp>

#include <rabbit/generated/shaders/forward.vert.spv.h>
#include <rabbit/generated/shaders/forward.frag.spv.h>

using namespace rb;

renderer::renderer(graphics_device& graphics_device) {
    matrices matrices;
    matrices.proj = rb::mat4f::perspective(45.0f, 1280.0f / 720.0f, 0.1f, 100.0f);
    matrices.view = rb::mat4f::translation({ 0.0f, 0.0f, -10.0f });
    matrices.world = rb::mat4f::identity();

    buffer_desc buffer_desc;
    buffer_desc.type = buffer_type::uniform;
    buffer_desc.size = sizeof(matrices);
    buffer_desc.stride = sizeof(matrices);
    buffer_desc.data = &matrices;
    _matrices_buffer = graphics_device.make_buffer(buffer_desc);

    material_desc material_desc;
    material_desc.vertex_desc = {
        { vertex_attribute::position, vertex_format::vec3f() },
        { vertex_attribute::texcoord, vertex_format::vec2f() },
        { vertex_attribute::normal, vertex_format::vec3f() },
    };
    material_desc.vertex_bytecode = forward_vert;
    material_desc.fragment_bytecode = forward_frag;
    material_desc.bindings = {
        { material_binding_type::uniform_buffer, shader_stage_flags::vertex, 0, 1 },
        { material_binding_type::texture, shader_stage_flags::fragment, 1, 1 },
    };
    _forward_material = graphics_device.make_material(material_desc);

    // resource_heap_desc resource_heap_desc;
    // resource_heap_desc.material = _forward_material;
    // resource_heap_desc.resource_views = {
    //     { material_binding_type::uniform_buffer, _matrices_buffer },
    // };
}

void renderer::draw(registry& registry, graphics_device& graphics_device) {
    graphics_device.begin_render_pass();

    registry.view<transform, geometry>().each([&](transform& transform, geometry& geometry) {
        if (!geometry.mesh) {
            return;
        }

        if (!geometry.albedo_map) {
            return;
        }

        if (!geometry.material) {
            geometry.material = _forward_material;
        }

        if (!geometry.resource_heap) {
            resource_heap_desc resource_heap_desc;
            resource_heap_desc.material = _forward_material;
            resource_heap_desc.resource_views = {
                { material_binding_type::uniform_buffer, _matrices_buffer },
                { material_binding_type::texture, nullptr, geometry.albedo_map },
            };
            geometry.resource_heap = graphics_device.make_resource_heap(resource_heap_desc);
        }

        graphics_device.draw(geometry.mesh, geometry.material, geometry.resource_heap);
    });

    graphics_device.end_render_pass();
}
