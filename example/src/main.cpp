#include <rabbit/rabbit.hpp>

#include <filesystem>

struct initialize_system : public rb::system {
    initialize_system(rb::asset_manager& asset_manager)
        : _asset_manager(asset_manager) {
    }

    void initialize(rb::registry& registry) override {
        auto base_mesh = _asset_manager.load<rb::mesh>("meshes/rabbit_base.obj");
        auto sphere_mesh = _asset_manager.load<rb::mesh>("meshes/rabbit_sphere.obj");

        auto base_texture = _asset_manager.load<rb::texture>("textures/rabbit_base.png");
        auto sphere_texture = _asset_manager.load<rb::texture>("textures/rabbit_sphere.png");

        auto base_entity = registry.create();
        auto sphere_entity = registry.create();

        registry.emplace<rb::transform>(base_entity);
        registry.emplace<rb::transform>(sphere_entity);

        registry.emplace<rb::geometry>(base_entity, base_mesh, nullptr, nullptr, base_texture);
        registry.emplace<rb::geometry>(sphere_entity, sphere_mesh, nullptr, nullptr, sphere_texture);
    }

    rb::asset_manager& _asset_manager;
};

int main(int argc, char* argv[]) {
    std::filesystem::current_path(DATA_DIRECTORY);

    auto app = rb::make_builder()
        .system<initialize_system>()
        .build();

    app.run();
}
