#include <rabbit/game.hpp>
#include <rabbit/clock.hpp>
#include <rabbit/texture_loader.hpp>
#include <rabbit/exception.hpp>

#include <chrono>
#include <thread>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

using namespace rb;

static game* instance = nullptr;

game::game(rb::config& config)
    : _config(config) {
    instance = this;

    _window = make_window(config);
    _keyboard = make_keyboard(config, _window);
    _mouse = make_mouse(config, _window);
    _graphics_device = make_graphics_device(config, _window);
    _gamepad = make_gamepad(config);
    _asset_manager = std::make_shared<rb::asset_manager>();
    _state_manager = std::make_shared<rb::state_manager>();

    _asset_manager->add_loader<texture>(std::make_shared<texture_loader>(_graphics_device));
}

void game_loop(void* user_data) {
    auto game = static_cast<rb::game*>(user_data);

    game->window()->poll_events();
    if (game->window()->is_focused()) {
        game->keyboard()->refresh();
        game->mouse()->refresh();
        game->gamepad()->refresh();
    }

    game->update(1.0f / 60.0f);

    game->state_manager()->update(1.0f / 60.0f);

    game->fixed_update(1.0f / 60.0f);

    game->state_manager()->fixed_update(1.0f / 60.0f);

    if (game->window()->is_focused()) {
        if (game->window()->size() != game->graphics_device()->backbuffer_size()) {
            game->graphics_device()->set_backbuffer_size(game->window()->size());
        }

        game->graphics_device()->set_render_target(nullptr);

        game->draw();

        game->state_manager()->draw();

        game->graphics_device()->present();
    }
}


void game::run() {
    initialize();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(&game_loop, this, 0, 1);
#else
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
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
        }
    }
#endif

    _state_manager->release();

    release();
}

void game::exit() {
    _running = false;
}

const rb::config& game::config() const {
    return _config;
}

std::shared_ptr<rb::window> game::window() const {
    return _window;
}

std::shared_ptr<rb::graphics_device> game::graphics_device() const {
    return _graphics_device;
}

std::shared_ptr<rb::keyboard> game::keyboard() const {
    return _keyboard;
}

std::shared_ptr<rb::mouse> game::mouse() const {
    return _mouse;
}

std::shared_ptr<rb::gamepad> game::gamepad() const {
    return _gamepad;
}

std::shared_ptr<rb::asset_manager> game::asset_manager() const {
    return _asset_manager;
}

std::shared_ptr<rb::state_manager> game::state_manager() const {
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
