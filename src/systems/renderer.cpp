#include <rabbit/systems/renderer.hpp>
#include <rabbit/components/transform.hpp>
#include <rabbit/components/geometry.hpp>
#include <rabbit/components/camera.hpp>

using namespace rb;

renderer::renderer(graphics_device& graphics_device) {
    camera_data camera_data;
    camera_data.proj = rb::mat4f::perspective(45.0f, 1280.0f / 720.0f, 0.1f, 100.0f);
    camera_data.view = rb::mat4f::translation({ 0.0f, -2.0f, -10.0f });

    buffer_desc buffer_desc;
    buffer_desc.type = buffer_type::uniform;
    buffer_desc.size = sizeof(camera_data);
    buffer_desc.stride = sizeof(camera_data);
    buffer_desc.data = &camera_data;
    _camera_buffer = graphics_device.make_buffer(buffer_desc);

    _forward_shader = graphics_device.make_shader(builtin_shader::forward);

    std::uint8_t pixels[] = { 255, 255, 255, 255 };

    texture_desc texture_desc;
    texture_desc.format = texture_format::rgba8;
    texture_desc.size = { 1, 1 };
    texture_desc.data = &pixels;
    _white_texture = graphics_device.make_texture(texture_desc);
    // material_desc material_desc;
    // material_desc.vertex_desc = {
    //     { vertex_attribute::position, vertex_format::vec3f() },
    //     { vertex_attribute::texcoord, vertex_format::vec2f() },
    //     { vertex_attribute::normal, vertex_format::vec3f() },
    // };
    // material_desc.vertex_bytecode = forward_vert;
    // material_desc.fragment_bytecode = forward_frag;
    // material_desc.bindings = {
    //     { material_binding_type::uniform_buffer, shader_stage_flags::vertex, 0, 1 },
    //     { material_binding_type::uniform_buffer, shader_stage_flags::vertex, 1, 1 },
    //     { material_binding_type::texture, shader_stage_flags::fragment, 2, 1 },
    // };
    // _forward_material = graphics_device.make_material(material_desc);
}

void renderer::draw(registry& registry, graphics_device& graphics_device) {
    // registry.view<transform, geometry>().each([&](transform& transform, geometry& geometry) {
    //     if (!geometry.mesh) {
    //         return;
    //     }

    //     if (!geometry.albedo_map) {
    //         return;
    //     }

    //     if (!geometry.material) {
    //         geometry.material = _forward_material;
    //     }

    //     local_data local_data;
    //     local_data.world = rb::mat4f::translation(transform.position);

    //     if (!geometry.local_buffer) {
    //         buffer_desc buffer_desc;
    //         buffer_desc.type = buffer_type::uniform;
    //         buffer_desc.size = sizeof(local_data);
    //         buffer_desc.stride = sizeof(local_data);
    //         buffer_desc.data = &local_data;
    //         geometry.local_buffer = graphics_device.make_buffer(buffer_desc);
    //     } else {
    //         graphics_device.update_buffer(geometry.local_buffer, &local_data, 0, sizeof(local_data));
    //     }

    //     if (!geometry.resource_heap) {
    //         resource_heap_desc resource_heap_desc;
    //         resource_heap_desc.material = _forward_material;
    //         resource_heap_desc.resource_views = {
    //             { material_binding_type::uniform_buffer, _camera_buffer },
    //             { material_binding_type::uniform_buffer, geometry.local_buffer },
    //             { material_binding_type::texture, nullptr, geometry.albedo_map },
    //         };
    //         geometry.resource_heap = graphics_device.make_resource_heap(resource_heap_desc);
    //     }
    // });

    graphics_device.begin();

    registry.view<transform, geometry>().each([&](const entity& entity, transform& transform, geometry& geometry) {
        if (!geometry.mesh || !geometry.material) {
            return;
        }

        local_data local_data;
        local_data.world = rb::mat4f::translation(transform.position);


        auto& entity_render_data = _entity_render_data[entity];
        if (!entity_render_data.local_buffer) {
            buffer_desc buffer_desc;
            buffer_desc.type = buffer_type::uniform;
            buffer_desc.size = sizeof(local_data);
            buffer_desc.stride = sizeof(local_data);
            buffer_desc.data = &local_data;
            entity_render_data.local_buffer = graphics_device.make_buffer(buffer_desc);

            material_data material_data;
            material_data.base_color = geometry.material->base_color();
            material_data.roughness = geometry.material->roughness();
            material_data.metallic = geometry.material->metallic();

            buffer_desc.type = buffer_type::uniform;
            buffer_desc.size = sizeof(material_data);
            buffer_desc.stride = sizeof(material_data);
            buffer_desc.data = &material_data;
            entity_render_data.material_buffer = graphics_device.make_buffer(buffer_desc);
        } else {
            graphics_device.update_buffer(entity_render_data.local_buffer, &local_data, 0, sizeof(local_data));
        }

        if (!entity_render_data.shader_data) {
            shader_data_desc desc;
            desc.shader = _forward_shader;
            desc.data = {
                { 0, _camera_buffer },
                { 1, entity_render_data.local_buffer },
                { 2, entity_render_data.material_buffer },
                { 3, geometry.material->albedo_map() ? geometry.material->albedo_map() : _white_texture }
            };
            entity_render_data.shader_data = graphics_device.make_shader_data(desc);
        }
    });

    registry.view<transform, geometry>().each([&](transform& transform, geometry& geometry) {
        if (!geometry.mesh || !geometry.material) {
            return;
        }

        local_data local_data;
        local_data.world = rb::mat4f::translation(transform.position);

        //  graphics_device.update_buffer(_matrix_buffer, &matrix_data, 0, sizeof(matrix_data));
    });

    graphics_device.begin_render_pass();
    registry.view<transform, geometry>().each([&](const entity& entity, transform& transform, geometry& geometry) {
        if (!geometry.mesh || !geometry.material) {
            return;
        }

        const auto& entity_render_data = _entity_render_data.at(entity);

        graphics_device.draw(geometry.mesh, _forward_shader, entity_render_data.shader_data);
    //    graphics_device.draw(geometry.mesh, geometry.material);
    });
    graphics_device.end_render_pass();

    graphics_device.end();
}
