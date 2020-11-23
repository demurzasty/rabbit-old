#include <rabbit/game.hpp>
#include <rabbit/clock.hpp>
#include <rabbit/texture_loader.hpp>
#include <rabbit/exception.hpp>

#include <chrono>
#include <iostream>

using namespace rb;

game::game(rb::config& config)
    : _config(config) {
    _window = make_window(config);
    _keyboard = make_keyboard(config);
    _mouse = make_mouse(config, _window);
    _graphics_device = make_graphics_device(config, _window);
    _gamepad = make_gamepad(config);
    _asset_manager = std::make_shared<rb::asset_manager>();
    _state_manager = std::make_shared<rb::state_manager>();

    _asset_manager->add_loader<texture>(std::make_shared<texture_loader>(_graphics_device));
}

void game::run() {
    initialize();

    clock clock;
    long double acc = 0.0L;
    while (_window->is_open() && _running) {
        _window->poll_events();

        if (_window->is_focused()) {
            _keyboard->refresh();
            _mouse->refresh();
            _gamepad->refresh();
        }

        const auto elapsed_time = clock.reset();

        update(static_cast<float>(elapsed_time));

        _state_manager->update(static_cast<float>(elapsed_time));

        acc += elapsed_time;
        while (acc >= _config.fixed_time_step) {
            fixed_update(static_cast<float>(_config.fixed_time_step));

            _state_manager->fixed_update(static_cast<float>(_config.fixed_time_step));

            acc -= _config.fixed_time_step;
        }

        if (_window->is_focused()) {
            if (_window->size() != _graphics_device->backbuffer_size()) {
                _graphics_device->set_backbuffer_size(_window->size());
            }

            _graphics_device->set_render_target(nullptr);

            draw();

            _state_manager->draw();

            _graphics_device->present();
        }
    }

    _state_manager->release();

    release();
}

void game::exit() {
    _running = false;
}

const config& game::config() const {
    return _config;
}

std::shared_ptr<window> game::window() const {
    return _window;
}

std::shared_ptr<graphics_device> game::graphics_device() const {
    return _graphics_device;
}

std::shared_ptr<keyboard> game::keyboard() const {
    return _keyboard;
}

std::shared_ptr<mouse> game::mouse() const {
    return _mouse;
}

std::shared_ptr<gamepad> game::gamepad() const {
    return _gamepad;
}

std::shared_ptr<asset_manager> game::asset_manager() const {
    return _asset_manager;
}

std::shared_ptr<state_manager> game::state_manager() const {
    return _state_manager;
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
