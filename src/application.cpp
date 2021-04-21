#include <rabbit/application.hpp>
#include <rabbit/builder.hpp>
#include <rabbit/window.hpp>
#include <rabbit/graphics_device.hpp>
#include <rabbit/mat4.hpp>
#include <rabbit/asset_manager.hpp>

#include <cstdint>
#include <rabbit/generated/shaders/forward.vert.spv.h>
#include <rabbit/generated/shaders/forward.frag.spv.h>

#include <chrono>

using namespace rb;

application::application(const builder& builder) {
    for (auto& installation : builder._installations) {
        installation(_injector);
    }

    for (auto& system_factory : builder._system_factories) {
        _systems.push_back(system_factory(_injector));
    }
}

int application::run() {
    auto& state = _injector.get<application_state>();
    auto& window = _injector.get<rb::window>();
    auto& graphics_device = _injector.get<rb::graphics_device>();

    while (state.running && window.is_open()) {
        window.poll_events();

        graphics_device.begin();

        for (auto& system : _systems) {
            system->draw(_registry, graphics_device);
        }

        graphics_device.end();

        graphics_device.present();
    }

    return 0;
}
