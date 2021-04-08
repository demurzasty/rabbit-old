#include <rabbit/rabbit.hpp>

#include <filesystem>
#include <iostream>

class test_system : public rb::system {
public:
    test_system(rb::asset_manager& asset_manager)
        : _asset_manager(asset_manager) {
    }

    void initialize(rb::registry& registry) override {
        auto base_texture = _asset_manager.load<rb::texture>("textures/rabbit_base.png");
        auto sphere_texture = _asset_manager.load<rb::texture>("textures/rabbit_sphere.png");

        auto base_material = std::make_shared<rb::material>();
        auto sphere_material = std::make_shared<rb::material>();

        base_material->diffuse_map = base_texture;
        sphere_material->diffuse_map = sphere_texture;
        sphere_material->diffuse = { 0.5f, 0.8f, 1.0f };
        sphere_material->metallic = 1.0f;
        sphere_material->roughness = 0.0f;

        auto mesh = _asset_manager.load<rb::mesh>("rabbit_base.obj");
        
        auto sphere = registry.create();
        registry.emplace<rb::geometry>(sphere, mesh, base_material);
        registry.emplace<rb::transform>(sphere);

        mesh = _asset_manager.load<rb::mesh>("rabbit_sphere.obj");
        sphere = registry.create();
        registry.emplace<rb::geometry>(sphere, mesh, sphere_material);
        registry.emplace<rb::transform>(sphere);
    }

private:
    rb::asset_manager& _asset_manager;
};

int main(int argc, char* argv[]) {
    std::filesystem::current_path(DATA_DIRECTORY);

    auto game = rb::make_default_builder()
        .system<test_system>()
        .build();
    
    return game.run();
}
