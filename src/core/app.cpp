#include <rabbit/rabbit.hpp>

using namespace rb;

std::list<std::function<void()>> app::_setups;
std::list<std::function<void()>> app::_inits;
std::list<std::function<void()>> app::_releases;
std::list<std::function<std::shared_ptr<rb::system>()>> app::_systems;

void app::setup() {
    _reflect();

    add_submodule<rb::time>();
    add_submodule<window>();
    add_submodule<graphics>();
    add_submodule<assets>();
    add_submodule<world>();

#if !RB_PROD_BUILD
    add_submodule<editor>();
#endif

    add_system<renderer>();

    add_initialization([] {
        assets::add_loader<texture, texture_loader>();
    });

#if !RB_PROD_BUILD
    add_initialization([] {
        editor::scan();
    });
#endif
}

void app::run() {
    for (const auto& setup : _setups) {
        setup();
    }

    for (const auto& init : _inits) {
        init();
    }

    std::unordered_map<system_stage, std::list<std::shared_ptr<rb::system>>> systems;
    for (const auto& system_factory : _systems) {
        auto system = system_factory();
        systems[system->stage()].push_back(system);
    }

    for (auto& [stage, system_list] : systems) {
        std::stable_sort(system_list.begin(), system_list.end(), [](auto a, auto b) {
            return a->priority() < b->priority();
        });
    }

    for (auto& system : systems[system_stage::init]) {
        system->process();
    }

    clock clock;
    auto accumulation_time = 0.0f;

    while (window::is_open()) {
        window::poll_events();

        time::_elapsed_time = clock.reset();
        for (auto& system : systems[system_stage::update]) {
            system->process();
        }

        accumulation_time += time::elapsed_time();
        while (accumulation_time >= time::fixed_time()) {
            for (auto& system : systems[system_stage::fixed_update]) {
                system->process();
            }

            accumulation_time -= time::fixed_time();
        }

        graphics::draw();

        graphics::swap_buffers();
    }

    systems.clear();

    for (const auto& release : _releases) {
        release();
    }
}
