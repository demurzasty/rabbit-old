#include <rabbit/application.hpp>
#include <rabbit/builder.hpp>
#include <rabbit/window.hpp>
#include <rabbit/graphics_device.hpp>
#include <rabbit/mat4.hpp>
#include <rabbit/asset_manager.hpp>

#include <cstdint>
#include <rabbit/generated/shaders/forward.vert.spv.h>
#include <rabbit/generated/shaders/forward.frag.spv.h>

#include <chrono>

using namespace rb;

application::application(const builder& builder) {
    for (auto& installation : builder._installations) {
        installation(_injector);
    }

    for (auto& system_factory : builder._system_factories) {
        _systems.push_back(system_factory(_injector));
    }
}

int application::run() {
    auto& state = _injector.get<application_state>();
    auto& window = _injector.get<rb::window>();
    auto& graphics_device = _injector.get<rb::graphics_device>();
    auto& asset_manager = _injector.get<rb::asset_manager>();

    vertex_desc vertex_desc = {
        { vertex_attribute::position, vertex_format::vec3f() },
        { vertex_attribute::texcoord, vertex_format::vec2f() },
        { vertex_attribute::normal, vertex_format::vec3f() }
    };

    struct vertex {
        vec3f position;
        vec2f texcoord;
        vec3f normal;
    };

    vertex vertices[] = {
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { { 0.0f, -1.0f, 0.0f }, { 0.5f, 1.0f }, { 0.0f, 0.0f, 1.0f } }
    };

    buffer_desc buffer_desc;
    buffer_desc.type = buffer_type::vertex;
    buffer_desc.data = vertices;
    buffer_desc.size = sizeof(vertices);
    buffer_desc.stride = sizeof(vertex);
    auto vertex_buffer = graphics_device.make_buffer(buffer_desc);

    auto texture = asset_manager.load<rb::texture>("C:/Users/Demurzasty/Projects/rabbit6/example/data/textures/rabbit_base.png");

    struct matrices {
        mat4f proj;
        mat4f view;
        mat4f world;
    };

    matrices matrices;
    matrices.proj = mat4f::identity();
    matrices.view = mat4f::identity();
    matrices.world = mat4f::identity();

    buffer_desc.type = buffer_type::uniform;
    buffer_desc.data = &matrices;
    buffer_desc.size = sizeof(matrices);
    buffer_desc.stride = sizeof(matrices);
    auto uniform_buffer = graphics_device.make_buffer(buffer_desc);

    material_desc material_desc;
    material_desc.vertex_desc = vertex_desc;
    material_desc.vertex_bytecode = forward_vert;
    material_desc.fragment_bytecode = forward_frag;
    material_desc.bindings = {
        { material_binding_type::uniform_buffer, shader_stage_flags::vertex, 0, 1 },
        { material_binding_type::texture, shader_stage_flags::fragment, 1, 1 }
    };
    auto material = graphics_device.make_material(material_desc);
 
    mesh_desc mesh_desc;
    mesh_desc.vertex_buffer = vertex_buffer;
    mesh_desc.vertex_desc = vertex_desc;
    // auto mesh = graphics_device.make_mesh(mesh_desc);
    auto mesh = asset_manager.load<rb::mesh>("C:/Users/Demurzasty/Projects/rabbit6/example/data/meshes/rabbit_base.obj");

    resource_heap_desc resource_heap_desc;
    resource_heap_desc.material = material;
    resource_heap_desc.resource_views = {
        { material_binding_type::uniform_buffer, uniform_buffer },
        { material_binding_type::texture, nullptr, texture }
    };
    auto resource_heap = graphics_device.make_resource_heap(resource_heap_desc);

    auto frame_index = 0;
    auto fps = 0;
    auto last_time = std::chrono::steady_clock::now();
    auto curr_time = std::chrono::steady_clock::now();

    while (state.running && window.is_open()) {
        window.poll_events();

        curr_time = std::chrono::steady_clock::now();

        auto diff = std::chrono::duration_cast<std::chrono::duration<float>>(curr_time - last_time).count();

        if (diff >= 1.0f) {
            char buffer[64];
            sprintf(buffer, "RabBit FPS: %d", fps);
            window.set_title(buffer);

            fps = 0;
            last_time = curr_time;
        }

        matrices.world = rb::mat4f::rotation_y(frame_index * 0.002f);
        matrices.proj = rb::mat4f::perspective(45.0f, 1280.0f / 720.0f, 0.1f, 100.0f);
       // matrices.proj[5] = -matrices.proj[5];
        matrices.view = rb::mat4f::translation({ 0.0f, -3.0f, -10.0f });

        graphics_device.begin();

        graphics_device.update_buffer(uniform_buffer, &matrices, 0, sizeof(matrices));

        graphics_device.begin_render_pass();

        graphics_device.draw(mesh, material, resource_heap);

        graphics_device.end_render_pass();

        graphics_device.end();

        graphics_device.present();

        frame_index++;
        fps++;
    }

    return 0;
}
