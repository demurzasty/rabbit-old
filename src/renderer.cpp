#include <rabbit/renderer.hpp>
#include <rabbit/graphics.hpp>
#include <rabbit/light.hpp>

using namespace rb;

void renderer::draw(registry& registry) {
	graphics::begin();

	registry.view<transform, camera>().each([this](transform& transform, camera& camera) {
		graphics::set_camera(transform, camera);
	});

	for (auto entity : registry.view<light>()) {
		auto& light = registry.get<rb::light>(entity);

		if (registry.any_of<directional_light>(entity)) {
			auto& [transform, directional_light] = registry.get<rb::transform, rb::directional_light>(entity);
			graphics::add_directional_light(transform, light, directional_light);
		}

		if (registry.any_of<point_light>(entity)) {
			auto& [transform, point_light] = registry.get<rb::transform, rb::point_light>(entity);
			graphics::add_point_light(transform, light, point_light);
		}
	}

	graphics::begin_shadow_pass();

	registry.view<transform, geometry>().each([](transform& transform, geometry& geometry) {
		graphics::draw_shadow(transform, geometry);
	});

	graphics::end_shadow_pass();

	graphics::begin_render_pass();

	registry.view<transform, geometry>().each([](transform& transform, geometry& geometry) {
		graphics::draw_geometry(transform, geometry);
	});

	registry.view<transform, camera>().each([this](transform& transform, camera& camera) {
		graphics::draw_skybox(camera.environment);
	});

	graphics::end_render_pass();

	graphics::end();
}
