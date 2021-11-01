#include <rabbit/renderer.hpp>
#include <rabbit/graphics.hpp>
#include <rabbit/light.hpp>
#include <rabbit/input.hpp>
#include <rabbit/settings.hpp>
#include <rabbit/camera_manager.hpp>

using namespace rb;

void renderer::initialize(registry& registry) {
    _viewport = graphics::make_viewport({ settings::window_size });
}

void renderer::update(registry& registry, float elapsed_time) {
    if (!registry.valid(camera_manager::main_camera)) {
        for (auto entity : registry.view<camera>()) {
            camera_manager::main_camera = entity;
            break;
        }
    }
}

void renderer::draw(registry& registry) {
    if (!registry.valid(camera_manager::main_camera)) {
        return;
    }

    if (!registry.all_of<transform, camera>(camera_manager::main_camera)) {
        return;
    }

    graphics::begin();

    const auto& [camera_transform, camera] = registry.get<transform, rb::camera>(camera_manager::main_camera);
    graphics::set_camera(camera_transform, camera);

    graphics::begin_geometry_pass(_viewport);

    registry.view<transform, geometry>().each([this](transform& transform, geometry& geometry) {
        graphics::draw_geometry(_viewport, transform, geometry);
    });

    graphics::end_geometry_pass(_viewport);

    registry.view<transform, light, directional_light>().each([this, &registry](transform& transform, light& light, directional_light& directional_light) {
        graphics::begin_shadow_pass(transform, light, directional_light);
        
        registry.view<rb::transform, geometry>().each([](rb::transform& transform, geometry& geometry) {
            graphics::draw_shadow(transform, geometry);
        });

        graphics::end_shadow_pass();
    });

    graphics::begin_light_pass(_viewport);

    graphics::draw_ambient(_viewport);

    registry.view<transform, light, directional_light>().each([this, &registry](transform& transform, light& light, directional_light& directional_light) {
        graphics::draw_directional_light(_viewport, transform, light, directional_light);
    });

    graphics::end_light_pass(_viewport);

    graphics::begin_forward_pass(_viewport);

    graphics::draw_skybox(_viewport);

    graphics::end_forward_pass(_viewport);

    graphics::begin_postprocess_pass(_viewport);

    graphics::draw_fxaa(_viewport);

    graphics::next_postprocess_pass(_viewport);

    graphics::draw_sharpen(_viewport, 0.25f);

    graphics::end_postprocess_pass(_viewport);

    graphics::present(_viewport);

    graphics::end();
}
