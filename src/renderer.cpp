#include <rabbit/renderer.hpp>
#include <rabbit/graphics.hpp>
#include <rabbit/light.hpp>

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

	for (auto entity : registry.view<transform, light>()) {
		auto& light = registry.get<rb::light>(entity);

		if (registry.any_of<directional_light>(entity)) {
			auto& [light_transform, directional_light] = registry.get<transform, rb::directional_light>(entity);
			graphics::begin_shadow_pass(light_transform, light, directional_light);

			registry.view<transform, geometry>().each([](transform& transform, geometry& geometry) {
				graphics::draw_shadow(transform, geometry);
			});

			graphics::end_shadow_pass();

			graphics::begin_render_pass();

			graphics::draw_directional_light(light_transform, light, directional_light);

			graphics::end_render_pass();
		}
	}

	graphics::begin_render_pass();

	graphics::draw_skybox();

	graphics::end_render_pass();

	graphics::end();
}
