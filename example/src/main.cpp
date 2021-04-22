#include <rabbit/rabbit.hpp>

#include <filesystem>

struct initialize_system : public rb::system {
    initialize_system(rb::asset_manager& asset_manager, rb::graphics_device& graphics_device)
        : _asset_manager(asset_manager)
        , _graphics_device(graphics_device) {
    }

    void initialize(rb::registry& registry) override {
        auto base_mesh = _asset_manager.load<rb::mesh>("meshes/rabbit_base.obj");
        auto sphere_mesh = _asset_manager.load<rb::mesh>("meshes/rabbit_sphere.obj");

        auto base_texture = _asset_manager.load<rb::texture>("textures/rabbit_base.png");
        auto sphere_texture = _asset_manager.load<rb::texture>("textures/rabbit_sphere.png");

        auto forward_shader = _graphics_device.make_shader(rb::builtin_shader::forward);

        rb::material_desc material_desc;
        material_desc.albedo_map = base_texture;
        auto base_material = _graphics_device.make_material(material_desc);

        material_desc.albedo_map = nullptr;
        material_desc.base_color = { 0.2f, 0.5f, 0.8f };
        material_desc.albedo_map = sphere_texture;
        auto sphere_material = _graphics_device.make_material(material_desc);

        for (auto index : rb::make_range(0, 3)) {
            auto base_entity = registry.create();
            auto sphere_entity = registry.create();

            registry.emplace<rb::transform>(base_entity).position = { -3.0f + index * 3.0f, 0.0f, 0.0f };
            registry.emplace<rb::transform>(sphere_entity).position = { -3.0f + index * 3.0f, 0.0f, 0.0f };

            registry.emplace<rb::geometry>(base_entity, base_mesh, base_material);
            registry.emplace<rb::geometry>(sphere_entity, sphere_mesh, sphere_material);
        }
    }

    rb::asset_manager& _asset_manager;
    rb::graphics_device& _graphics_device;
};

int main(int argc, char* argv[]) {
    std::filesystem::current_path(DATA_DIRECTORY);

    auto app = rb::make_builder()
        .system<initialize_system>()
        .build();

    app.run();
}
