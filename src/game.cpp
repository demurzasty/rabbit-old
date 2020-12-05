#include <rabbit/game.hpp>
#include <rabbit/clock.hpp>
#include <rabbit/texture_loader.hpp>
#include <rabbit/exception.hpp>

#include <chrono>
#include <thread>
#include <iostream>

using namespace rb;

game::game(std::shared_ptr<rb::config> config) {
    _container.install(config);

    _container.install<rb::window>(dependency_lifetime::singleton, &rb::window::resolve);
    _container.install<rb::keyboard>(dependency_lifetime::singleton, &rb::keyboard::resolve);
    _container.install<rb::mouse>(dependency_lifetime::singleton, &rb::mouse::resolve);
    _container.install<rb::graphics_device>(dependency_lifetime::singleton, &rb::graphics_device::resolve);
    _container.install<rb::gamepad>(dependency_lifetime::singleton, &rb::gamepad::resolve);
    _container.install<rb::mouse>(dependency_lifetime::singleton, &rb::mouse::resolve);
    _container.install<rb::asset_manager>(rb::dependency_lifetime::singleton);
    _container.install<rb::state_manager>(rb::dependency_lifetime::singleton);

    asset_manager()->add_loader<texture>(std::make_shared<texture_loader>(graphics_device()));
}

void game::run() {
    initialize();

    auto config = this->config();
    auto window = this->window();
    auto keyboard = this->keyboard();
    auto mouse = this->mouse();
    auto gamepad = this->gamepad();
    auto graphics_device = this->graphics_device();
    auto state_manager = this->state_manager();

    clock clock;
    long double acc = 0.0L;
    while (window->is_open() && _running) {
        window->poll_events();

        if (window->is_focused()) {
            keyboard->refresh();
            mouse->refresh();
            gamepad->refresh();
        }

        const auto elapsed_time = clock.reset();

        update(static_cast<float>(elapsed_time));

        state_manager->update(static_cast<float>(elapsed_time));

        acc += elapsed_time;
        while (acc >= config->fixed_time_step) {
            fixed_update(static_cast<float>(config->fixed_time_step));

            state_manager->fixed_update(static_cast<float>(config->fixed_time_step));

            acc -= config->fixed_time_step;
        }

        if (window->is_focused()) {
            if (window->size() != graphics_device->backbuffer_size()) {
                graphics_device->set_backbuffer_size(window->size());
            }

            graphics_device->set_render_target(nullptr);

            draw();

            state_manager->draw();

            graphics_device->present();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
        }
    }

    state_manager->release();

    release();
}

void game::exit() {
    _running = false;
}

std::shared_ptr<rb::config> game::config() const {
    return _container.get<rb::config>();
}

std::shared_ptr<rb::window> game::window() const {
    return _container.get<rb::window>();
}

std::shared_ptr<rb::graphics_device> game::graphics_device() const {
    return _container.get<rb::graphics_device>();
}

std::shared_ptr<rb::keyboard> game::keyboard() const {
    return _container.get<rb::keyboard>();
}

std::shared_ptr<rb::mouse> game::mouse() const {
    return _container.get<rb::mouse>();
}

std::shared_ptr<rb::gamepad> game::gamepad() const {
    return _container.get<rb::gamepad>();
}

std::shared_ptr<rb::asset_manager> game::asset_manager() const {
    return _container.get<rb::asset_manager>();
}

std::shared_ptr<rb::state_manager> game::state_manager() const {
    return _container.get<rb::state_manager>();
}

void game::initialize() {
}

void game::release() {
}

void game::update(float elapsed_time) {
}

void game::fixed_update(float fixed_time) {
}

void game::draw() {
}
