#include <rabbit/renderer.hpp>
#include <rabbit/graphics.hpp>
#include <rabbit/light.hpp>
#include <rabbit/input.hpp>
#include <rabbit/settings.hpp>

using namespace rb;

void renderer::initialize(registry& registry) {
    _viewport = graphics::make_viewport({ settings::window_size });
    _viewport->motion_blur_enabled = false;
}

void renderer::update(registry& registry, float elapsed_time) {
    if (input::is_key_pressed(keycode::m)) {
        _viewport->motion_blur_enabled = !_viewport->motion_blur_enabled;
    }

    if (!registry.valid(_viewport->camera)) {
        for (auto entity : registry.view<camera>()) {
            _viewport->camera = entity;
            break;
        }
    }
}

void renderer::draw(registry& registry) {
    if (!registry.valid(_viewport->camera)) {
        return;
    }

    if (!registry.all_of<transform, camera>(_viewport->camera)) {
        return;
    }

    graphics::begin();

    const auto& [camera_transform, camera] = registry.get<transform, rb::camera>(_viewport->camera);
    graphics::set_camera(camera_transform, camera);

    graphics::begin_geometry_pass(_viewport);

    registry.view<transform, geometry>().each([this](transform& transform, geometry& geometry) {
        graphics::draw_geometry(_viewport, transform, geometry);
    });

    graphics::end_geometry_pass(_viewport);

    registry.view<transform, light, directional_light>().each([this, &registry](transform& transform, light& light, directional_light& directional_light) {
        if (!directional_light.shadow_enabled) {
            return;
        }

        for (auto cascade = 0u; cascade < graphics_limits::max_shadow_cascades; ++cascade) {
            graphics::begin_shadow_pass(transform, light, directional_light, cascade);
        
            registry.view<rb::transform, geometry>().each([cascade](rb::transform& transform, geometry& geometry) {
                graphics::draw_shadow(transform, geometry, cascade);
            });

            graphics::end_shadow_pass();
        }
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

    if (_viewport->fxaa_enabled) {
        graphics::next_postprocess_pass(_viewport);

        graphics::draw_fxaa(_viewport);
    }

    if (_viewport->sharpen_enabled) {
        graphics::next_postprocess_pass(_viewport);

        graphics::draw_sharpen(_viewport, _viewport->sharpen_factor);
    }

    if (_viewport->motion_blur_enabled) {
        graphics::next_postprocess_pass(_viewport);

        graphics::draw_motion_blur(_viewport);
    }

    graphics::end_postprocess_pass(_viewport);

    graphics::present(_viewport);

    graphics::end();
}
