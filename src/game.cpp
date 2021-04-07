#include <rabbit/game.hpp>
#include <rabbit/clock.hpp>
#include <rabbit/texture_loader.hpp>
#include <rabbit/exception.hpp>

#include <chrono>
#include <thread>
#include <iostream>

using namespace rb;

game::game(const builder& builder) {
    // Install services in container.
    for (auto& service : builder._services) {
        service(_container);
    }

    // Initialize all installed services.
    for (auto& initializer : builder._initializers) {
        initializer(_container);
    }

    // Create all systems using container.
    for (auto& system : builder._systems) {
        _systems.push_back(system(_container));
    }
}

// game::game(rb::config& config)
//     : _config(config) {
//     // _window = make_window(config);
//     // _keyboard = make_keyboard(config, _window);
//     // _mouse = make_mouse(config, _window);
//     // _graphics_device = make_graphics_device(config, _window);
//     // _gamepad = make_gamepad(config);
//     // _asset_manager = std::make_shared<rb::asset_manager>();
//     // _state_manager = std::make_shared<rb::state_manager>();

//     // _asset_manager->add_loader<texture>(std::make_shared<texture_loader>(_graphics_device));
// }

int game::run() {
    auto& window = _container.get<rb::window>();
    auto& graphics_device = _container.get<rb::graphics_device>();
    auto& keyboard = _container.get<rb::keyboard>();
    auto& mouse = _container.get<rb::mouse>();
    auto& gamepad = _container.get<rb::gamepad>();
    auto& config = _container.get<rb::config>();

    clock clock;
    auto acc = 0.0f;
    while (window.is_open()) {
        window.poll_events();

        if (window.is_focused()) {
            keyboard.refresh();
            mouse.refresh();
            gamepad.refresh();
        }

        const auto elapsed_time = static_cast<float>(clock.reset());

        for (auto& system : _systems) {
            system->variable_update(_registry, elapsed_time);
        }

        acc += elapsed_time;
        while (acc >= config.fixed_time_step) {
            for (auto& system : _systems) {
                system->fixed_update(_registry, static_cast<float>(config.fixed_time_step));
            }
            acc -= config.fixed_time_step;
        }

        if (window.is_focused()) {
            if (window.size() != graphics_device.backbuffer_size()) {
                graphics_device.set_backbuffer_size(window.size());
            }

            graphics_device.set_render_target(nullptr);

             for (auto& system : _systems) {
                system->draw(_registry, graphics_device);
            }

            graphics_device.present();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
        }
    }
    return 0;
}
