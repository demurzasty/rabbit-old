#include <rabbit/engine/game.hpp>
#include <rabbit/core/clock.hpp>
#include <rabbit/loaders/texture_loader.hpp>
#include <rabbit/core/exception.hpp>

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

int game::run() {
    auto& window = _container.get<rb::window>();
    auto& graphics_device = _container.get<rb::graphics_device>();
    auto& keyboard = _container.get<rb::keyboard>();
    auto& mouse = _container.get<rb::mouse>();
    auto& gamepad = _container.get<rb::gamepad>();
    auto& config = _container.get<rb::config>();

    // Initialize all systems.
    for (auto& system : _systems) {
        system->initialize(_registry);
    }

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
