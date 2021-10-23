#include <rabbit/rabbit.hpp>

using namespace rb;

class initializer : public rb::system {
public:
	void initialize(registry& registry) override {
		auto coat = registry.create();
		auto& coat_transform = registry.emplace<transform>(coat);
		auto& coat_geometry = registry.emplace<geometry>(coat);

		coat_geometry.mesh = assets::load<mesh>("data/meshes/coat.obj");
		coat_geometry.material = assets::load<material>("data/materials/coat.json");

		auto main_camera = registry.create();
		registry.emplace<camera>(main_camera).environment = assets::load<environment>("data/cubemaps/magic_hour.json");
		registry.emplace<transform>(main_camera).position = { 0.0f, 1.0f, 5.0f };
	}
};

struct camera_controller : public rb::system {
	void update(registry& registry, float elapsed_time) {
		if (input::is_mouse_button_pressed(mouse_button::right)) {
			_last_mouse_position = input::mouse_position();
		}

		registry.view<transform, camera>().each([this, elapsed_time](transform& transform, camera& camera) {
			if (input::is_mouse_button_down(mouse_button::right)) {
				const auto mouse_position = input::mouse_position();
				const auto diff = _last_mouse_position - mouse_position;
				_last_mouse_position = mouse_position;

				transform.rotation.y += diff.x * 0.005f;
				transform.rotation.x += diff.y * 0.005f;

				if (input::is_key_down(keycode::e)) {
					transform.position.y += elapsed_time;
				} else if (input::is_key_down(keycode::q)) {
					transform.position.y -= elapsed_time;
				}

				if (input::is_key_down(keycode::w)) {
					transform.position.x -= std::sin(transform.rotation.y) * elapsed_time;
					transform.position.z -= std::cos(transform.rotation.y) * elapsed_time;
				} else if (input::is_key_down(keycode::s)) {
					transform.position.x += std::sin(transform.rotation.y) * elapsed_time;
					transform.position.z += std::cos(transform.rotation.y) * elapsed_time;
				}

				if (input::is_key_down(keycode::d)) {
					transform.position.x -= std::sin(transform.rotation.y - pi<float>() * 0.5f) * elapsed_time;
					transform.position.z -= std::cos(transform.rotation.y - pi<float>() * 0.5f) * elapsed_time;
				} else if (input::is_key_down(keycode::a)) {
					transform.position.x -= std::sin(transform.rotation.y + pi<float>() * 0.5f) * elapsed_time;
					transform.position.z -= std::cos(transform.rotation.y + pi<float>() * 0.5f) * elapsed_time;
				}
			}
		});
	}
private:
	vec2i _last_mouse_position;
};

int main(int argc, char* argv[]) {
	app::setup();

	app::system<camera_controller>();
	app::system<initializer>();

	app::run();
}
