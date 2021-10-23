#include <rabbit/renderer.hpp>
#include <rabbit/graphics.hpp>

using namespace rb;

void renderer::draw(registry& registry) {
	graphics::begin();

	registry.view<transform, camera>().each([this](transform& transform, camera& camera) {
		graphics::set_camera(transform, camera);
	});

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
