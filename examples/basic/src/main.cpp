#include <rabbit/rabbit.hpp>

#include <filesystem>
#include <iostream>

class test_system : public rb::system {
public:
    test_system(rb::asset_manager& asset_manager, rb::gamepad& gamepad)
        : _asset_manager(asset_manager)
        , _gamepad(gamepad) {
    }

    void initialize(rb::registry& registry) override {
        auto base_texture = _asset_manager.load<rb::texture>("textures/rabbit_base.png");
        auto sphere_texture = _asset_manager.load<rb::texture>("textures/rabbit_sphere.png");

        auto base_mesh = _asset_manager.load<rb::mesh>("meshes/rabbit_base.obj");
        auto sphere_mesh = _asset_manager.load<rb::mesh>("meshes/rabbit_sphere.obj");

        auto base_material = std::make_shared<rb::material>();
        base_material->diffuse_map = base_texture;

        for (auto i = 0; i < 5; ++i) {
            for (auto j = 0; j < 5; ++j) {
                auto sphere_material = std::make_shared<rb::material>();

                sphere_material->diffuse_map = sphere_texture;
                sphere_material->diffuse = { 0.5f, 0.8f, 1.0f };
                sphere_material->metallic = j / 4.0f;
                sphere_material->roughness = i / 4.0f;

                auto base = registry.create();
                registry.emplace<rb::geometry>(base, base_mesh, base_material);
                registry.emplace<rb::transform>(base).position = { i * 4.0f, 0.0f, j * 4.0f };

                auto sphere = registry.create();
                registry.emplace<rb::geometry>(sphere, sphere_mesh, sphere_material);
                registry.emplace<rb::transform>(sphere).position = { i * 4.0f, 0.0f, j * 4.0f };
            }
        }

        auto helmet_material = std::make_shared<rb::material>();
        helmet_material->diffuse_map = _asset_manager.load<rb::texture>("textures/helmet/default_albedo_ao.png");
        helmet_material->normal_map = _asset_manager.load<rb::texture>("textures/helmet/Default_normal.jpg");
        helmet_material->roughness_map = _asset_manager.load<rb::texture>("textures/helmet/Default_roughness.png");
        helmet_material->metallic_map = _asset_manager.load<rb::texture>("textures/helmet/Default_metal.png");
        helmet_material->emissive_map = _asset_manager.load<rb::texture>("textures/helmet/Default_emissive.jpg");
        helmet_material->roughness = 1.0f;
        helmet_material->metallic = 1.0f;

        auto helmet_mesh = _asset_manager.load<rb::mesh>("meshes/helmet.obj");

        auto helmet = registry.create();
        registry.emplace<rb::geometry>(helmet, helmet_mesh, helmet_material);
        registry.emplace<rb::transform>(helmet).position = { -6.0f, 0.0f, 0.0f };

        auto camera = registry.create();
        registry.emplace<rb::camera>(camera);
        registry.emplace<rb::transform>(camera).position = { 0.0f, 0.0f, 10.0f };
    }

    void variable_update(rb::registry& registry, float elapsed_time) override {
        const auto horizontal = _gamepad.axis(rb::gamepad_player::first, rb::gamepad_axis::right_x);
        const auto horizontal_left = _gamepad.axis(rb::gamepad_player::first, rb::gamepad_axis::left_x);
        const auto vertical = _gamepad.axis(rb::gamepad_player::first, rb::gamepad_axis::left_y);
        const auto vertical_right = _gamepad.axis(rb::gamepad_player::first, rb::gamepad_axis::right_y);
        const auto up = _gamepad.axis(rb::gamepad_player::first, rb::gamepad_axis::right_trigger);
        const auto down = _gamepad.axis(rb::gamepad_player::first, rb::gamepad_axis::left_trigger);
        const auto speed = _gamepad.is_button_down(rb::gamepad_player::first, rb::gamepad_button::left_bumper) ? 20.0f : 2.0f;
        

        registry.view<rb::camera, rb::transform>().each([&](rb::camera& camera, rb::transform& transform) {
            if (rb::abs(horizontal) > 0.1) {
                transform.rotation.y -= horizontal * elapsed_time * 2.0f;
            }

            if (rb::abs(horizontal_left) > 0.1) {
                transform.position.x += std::sin(transform.rotation.y + rb::pi<float>() * 0.5) * horizontal_left * elapsed_time * 2.0f;
                transform.position.z += std::cos(transform.rotation.y + rb::pi<float>() * 0.5) * horizontal_left * elapsed_time * 2.0f;
            }

            if (rb::abs(vertical) > 0.1) {
                transform.position.x += std::sin(transform.rotation.y) * vertical * elapsed_time * speed;
                transform.position.z += std::cos(transform.rotation.y) * vertical * elapsed_time * speed;
            }

            if (rb::abs(vertical_right) > 0.2) {
                transform.rotation.x -= vertical_right * elapsed_time * 2.0f;
            }

            if (rb::abs(up) > 0.1) {
                transform.position.y += up * elapsed_time * 2.0f;
            } else if (rb::abs(down) > 0.1) {
                transform.position.y -= down * elapsed_time * 2.0f;
            }
        });
    }

private:
    rb::asset_manager& _asset_manager;
    rb::gamepad& _gamepad;
};

int main(int argc, char* argv[]) {
    std::filesystem::current_path(DATA_DIRECTORY);

    auto game = rb::make_default_builder()
        .system<test_system>()
        .build();
    
    return game.run();
}
