#include <rabbit/renderer.hpp>
#include <rabbit/graphics.hpp>
#include <rabbit/light.hpp>
#include <rabbit/input.hpp>

using namespace rb;

void renderer::draw(registry& registry) {
    graphics::begin();

    registry.view<transform, camera>().each([this](transform& transform, camera& camera) {
        graphics::set_camera(transform, camera);
    });

    graphics::begin_geometry_pass();

    registry.view<transform, geometry>().each([](transform& transform, geometry& geometry) {
        graphics::draw_geometry(transform, geometry);
    });

    graphics::end_geometry_pass();

    graphics::begin_render_pass();

    graphics::draw_ambient();

    graphics::end_render_pass();

    registry.view<transform, light, directional_light>().each([&registry](transform& transform, light& light, directional_light& directional_light) {
        graphics::begin_shadow_pass(transform, light, directional_light);

        registry.view<rb::transform, geometry>().each([](rb::transform& transform, geometry& geometry) {
            graphics::draw_shadow(transform, geometry);
        });

        graphics::end_shadow_pass();

        graphics::begin_render_pass();

        graphics::draw_directional_light(transform, light, directional_light);

        graphics::end_render_pass();
    });

    graphics::begin_render_pass();

    registry.view<transform, light, point_light>().each([](transform& transform, light& light, point_light& point_light) {
        graphics::draw_point_light(transform, light, point_light);
    });

    // graphics::draw_ssao();

    graphics::draw_skybox();

    graphics::end_render_pass();

    graphics::end();
}
