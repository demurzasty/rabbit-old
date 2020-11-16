#include "mouse_win32.hpp"

#include <rabbit/enum.hpp>

#include <Windows.h>

using namespace rb;

mouse_win32::mouse_win32(std::shared_ptr<window> window)
    : _window(window), _state(), _last_state() {
}

void mouse_win32::refresh() {
    std::memcpy(_last_state, _state, sizeof(_state));
    
    _state[enum_size(mouse_button::left)] = static_cast<std::uint8_t>(GetKeyState(VK_LBUTTON));
    _state[enum_size(mouse_button::middle)] = static_cast<std::uint8_t>(GetKeyState(VK_MBUTTON));
    _state[enum_size(mouse_button::right)] = static_cast<std::uint8_t>(GetKeyState(VK_RBUTTON));
}

vec2i mouse_win32::position() {
    POINT position;
    if (GetCursorPos(&position) && ScreenToClient(_window->native_handle(), &position)) {
        return { position.x, position.y };
    }
    return { 0, 0 };
}

bool mouse_win32::is_button_down(mouse_button button) {
    return _state[enum_size(button)] & 0x80;
}

bool mouse_win32::is_button_up(mouse_button button) {
    return (_state[enum_size(button)] & 0x80) == 0;
}

bool mouse_win32::is_button_pressed(mouse_button button) {
    return (_state[enum_size(button)] & 0x80) && ((_last_state[enum_size(button)] & 0x80) == 0);
}

bool mouse_win32::is_button_released(mouse_button button) {
    return ((_state[enum_size(button)] & 0x80) == 0) && (_last_state[enum_size(button)] & 0x80);
}