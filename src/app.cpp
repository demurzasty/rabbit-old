#include <rabbit/rabbit.hpp>

#include <chrono>

using namespace rb;

std::list<void(*)()> app::_preinits;
std::list<void(*)()> app::_inits;
std::list<void(*)()> app::_releases;
std::list<std::shared_ptr<rb::system>(*)()> app::_systems;

void app::setup() {
	app::submodule<window>();
	app::submodule<input>();
	app::submodule<graphics>();
	app::submodule<assets>();

	app::init([] {
		assets::add_loader<texture>(&texture::load);
		assets::add_loader<environment>(&environment::load);
		assets::add_loader<material>(&material::load);
		assets::add_loader<mesh>(&mesh::load);
	});

	app::system<renderer>();
}

void app::run() {
	for (auto& preinit : _preinits) {
		preinit();
	}

	for (auto& init : _inits) {
		init();
	}
	
	_main_loop();

	for (auto& release : _releases) {
		release();
	}
}

void app::_main_loop() {
	std::list<std::shared_ptr<rb::system>> systems;
	std::transform(_systems.begin(), _systems.end(), std::back_inserter(systems), [](auto func) {
		return func();
	});

	registry registry;
	for (auto& system : systems) {
		system->initialize(registry);
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

		graphics::present();
	}

	graphics::flush();
}
