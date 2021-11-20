#include <rabbit/rabbit.hpp>

#include <chrono>

using namespace rb;

std::list<void(*)()> app::_preinits;
std::list<void(*)()> app::_inits;
std::list<void(*)()> app::_releases;
std::list<std::shared_ptr<rb::system>(*)()> app::_systems;
std::unordered_map<std::string, void(*)(registry&, entity, json_read_visitor&)> app::_deserializers;

void app::setup() {
	app::add_submodule<window>();
	app::add_submodule<input>();
	app::add_submodule<graphics>();
	app::add_submodule<assets>();

#if !RB_PROD_BUILD
	app::add_submodule<editor>();
#endif

	app::add_component<identity>("identity");
	app::add_component<transform>("transform");
	app::add_component<camera>("camera");
	app::add_component<geometry>("geometry");
	app::add_component<light>("light");
	app::add_component<directional_light>("directional_light");
	app::add_component<point_light>("point_light");

#if !RB_PROD_BUILD
	app::add_init([] {
		editor::scan();
	});
#endif

	app::add_init([] {
		assets::load_resources();
		assets::add_loader<texture>(&texture::load);
		assets::add_loader<environment>(&environment::load);
		assets::add_loader<material>(&material::load);
		assets::add_loader<mesh>(&mesh::load);
		assets::add_loader<prefab>(&prefab::load);
	});

	app::add_system<hierarchy>();
	app::add_system<renderer>();
}

void app::run(std::string initial_scene) {
	for (auto& preinit : _preinits) {
		preinit();
	}

	for (auto& init : _inits) {
		init();
	}
	
	_main_loop(initial_scene);

	for (auto& release : _releases) {
		release();
	}
}

void app::_main_loop(const std::string& initial_scene) {
	std::list<std::shared_ptr<rb::system>> systems;
	std::transform(_systems.begin(), _systems.end(), std::back_inserter(systems), [](auto func) {
		return func();
	});

	registry registry;
	for (auto& system : systems) {
		system->initialize(registry);
	}

	if (!initial_scene.empty()) {
		auto scene = assets::load<prefab>(initial_scene);
		scene->apply(registry, null);
	}

	auto last_time = std::chrono::steady_clock::now();
	auto accumulation_time = 0.0f;

	while (window::is_open()) {
		window::poll_events();
		input::refresh();

		const auto current_time = std::chrono::steady_clock::now();
		const auto elapsed_time = std::chrono::duration_cast<std::chrono::duration<float>>(current_time - last_time).count();
		last_time = current_time;

		for (auto& system : systems) {
			system->update(registry, elapsed_time);
		}

		for (auto& system : systems) {
			system->draw(registry);
		}

		graphics::swap_buffers();
	}

	graphics::flush();
}

app::deserializer app::get_deserializer(const std::string& name) {
	return _deserializers.at(name);
}
