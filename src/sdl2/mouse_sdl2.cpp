#include "mouse_sdl2.hpp"
#include "window_sdl2.hpp"

#include <rabbit/enum.hpp>

#include <map>

using namespace rb;

static std::map<mouse_button, Uint32> flags = {
    { mouse_button::left, SDL_BUTTON_LEFT },
    { mouse_button::middle, SDL_BUTTON_MIDDLE },
    { mouse_button::right, SDL_BUTTON_RIGHT }
};

mouse_sdl2::mouse_sdl2(window& window)
    : _window(window) {
}

void mouse_sdl2::refresh() {
    _last_state = _state;
    _state = SDL_GetMouseState(&_mouse_position.x, &_mouse_position.y);
}

vec2i mouse_sdl2::position() {
    return _mouse_position;
}

float mouse_sdl2::wheel() {
    return static_cast<window_sdl2&>(_window).mouse_wheel();
}

bool mouse_sdl2::is_button_down(mouse_button button) {
    return _state & flags.at(button);
}

bool mouse_sdl2::is_button_up(mouse_button button) {
    return (_state & flags.at(button)) == 0;
}

bool mouse_sdl2::is_button_pressed(mouse_button button) {
    return (_state & flags.at(button)) && ((_last_state & flags.at(button)) == 0);
}

bool mouse_sdl2::is_button_released(mouse_button button) {
    return ((_state & flags.at(button)) == 0) && (_last_state & flags.at(button));
}
