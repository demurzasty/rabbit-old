#include "mouse_sdl2.hpp"

#include <rabbit/enum.hpp>
#include <rabbit/exception.hpp>

#include <fmt/format.h>

using namespace rb;

static Uint32 flags[] = {
    SDL_BUTTON_LEFT,
    SDL_BUTTON_MIDDLE,
    SDL_BUTTON_RIGHT
};

mouse_sdl2::mouse_sdl2(std::shared_ptr<window> window)
    : _window(window) {
}

void mouse_sdl2::refresh() {
    _last_state = _state;
    _state = SDL_GetMouseState(&_mouse_position.x, &_mouse_position.y);
}

vec2i mouse_sdl2::position() {
    return _mouse_position;
}

bool mouse_sdl2::is_button_down(mouse_button button) {
    if (button == mouse_button::unknown || button == mouse_button::count) {
        throw exception{ fmt::format("Incorrect mouse button: {}", button) };
    }

    return _state & flags[enum_size(button)];
}

bool mouse_sdl2::is_button_up(mouse_button button) {
    if (button == mouse_button::unknown || button == mouse_button::count) {
        throw exception{ fmt::format("Incorrect mouse button: {}", button) };
    }

    return (_state & flags[enum_size(button)]) == 0;
}

bool mouse_sdl2::is_button_pressed(mouse_button button) {
    if (button == mouse_button::unknown || button == mouse_button::count) {
        throw exception{ fmt::format("Incorrect mouse button: {}", button) };
    }

    return (_state & flags[enum_size(button)]) && ((_last_state & flags[enum_size(button)]) == 0);
}

bool mouse_sdl2::is_button_released(mouse_button button) {
    if (button == mouse_button::unknown || button == mouse_button::count) {
        throw exception{ fmt::format("Incorrect mouse button: {}", button) };
    }

    return ((_state & flags[enum_size(button)]) == 0) && (_last_state & flags[enum_size(button)]);
}
