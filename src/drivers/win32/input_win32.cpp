#include "input_win32.hpp"
#include "window_win32.hpp"

#include <Windows.h>

#include <unordered_map>

using namespace rb;

static std::unordered_map<keycode, int> key_map = {
    { keycode::left_system, VK_LWIN },
    { keycode::right_system, VK_RWIN },
    { keycode::menu, VK_APPS },
    { keycode::semicolon, VK_OEM_1 },
    { keycode::slash, VK_OEM_2 },
    { keycode::equal, VK_OEM_PLUS },
    { keycode::minus, VK_OEM_MINUS },
    { keycode::left_bracket, VK_OEM_4 },
    { keycode::right_bracket, VK_OEM_6 },
    { keycode::comma, VK_OEM_COMMA },
    { keycode::period, VK_OEM_PERIOD },
    { keycode::quote, VK_OEM_7 },
    { keycode::backslash, VK_OEM_5 },
    { keycode::tilde, VK_OEM_3 },
    { keycode::escape, VK_ESCAPE },
    { keycode::space, VK_SPACE },
    { keycode::enter, VK_RETURN },
    { keycode::backspace, VK_BACK },
    { keycode::tab, VK_TAB },
    { keycode::page_up, VK_PRIOR },
    { keycode::page_down, VK_NEXT },
    { keycode::end, VK_END },
    { keycode::home, VK_HOME },
    { keycode::insert, VK_INSERT },
    { keycode::del, VK_DELETE },
    { keycode::add, VK_ADD },
    { keycode::subtract, VK_SUBTRACT },
    { keycode::multiply, VK_MULTIPLY },
    { keycode::divide, VK_DIVIDE },
    { keycode::pause, VK_PAUSE },
    { keycode::f1, VK_F1 },
    { keycode::f2, VK_F2 },
    { keycode::f3, VK_F3 },
    { keycode::f4, VK_F4 },
    { keycode::f5, VK_F5 },
    { keycode::f6, VK_F6 },
    { keycode::f7, VK_F7 },
    { keycode::f8, VK_F8 },
    { keycode::f9, VK_F9 },
    { keycode::f10, VK_F10 },
    { keycode::f11, VK_F11 },
    { keycode::f12, VK_F12 },
    { keycode::f13, VK_F13 },
    { keycode::f14, VK_F14 },
    { keycode::f15, VK_F15 },
    { keycode::left, VK_LEFT },
    { keycode::right, VK_RIGHT },
    { keycode::up, VK_UP },
    { keycode::down, VK_DOWN },
    { keycode::numpad0, VK_NUMPAD0 },
    { keycode::numpad1, VK_NUMPAD1 },
    { keycode::numpad2, VK_NUMPAD2 },
    { keycode::numpad3, VK_NUMPAD3 },
    { keycode::numpad4, VK_NUMPAD4 },
    { keycode::numpad5, VK_NUMPAD5 },
    { keycode::numpad6, VK_NUMPAD6 },
    { keycode::numpad7, VK_NUMPAD7 },
    { keycode::numpad8, VK_NUMPAD8 },
    { keycode::numpad9, VK_NUMPAD9 },
    { keycode::a, 'A' },
    { keycode::b, 'B' },
    { keycode::c, 'C' },
    { keycode::d, 'D' },
    { keycode::e, 'E' },
    { keycode::f, 'F' },
    { keycode::g, 'G' },
    { keycode::h, 'H' },
    { keycode::i, 'I' },
    { keycode::j, 'J' },
    { keycode::k, 'K' },
    { keycode::l, 'L' },
    { keycode::m, 'M' },
    { keycode::n, 'N' },
    { keycode::o, 'O' },
    { keycode::p, 'P' },
    { keycode::q, 'Q' },
    { keycode::r, 'R' },
    { keycode::s, 'S' },
    { keycode::t, 'T' },
    { keycode::u, 'U' },
    { keycode::v, 'V' },
    { keycode::w, 'W' },
    { keycode::x, 'X' },
    { keycode::y, 'Y' },
    { keycode::z, 'Z' },
    { keycode::num0, '0' },
    { keycode::num1, '1' },
    { keycode::num2, '2' },
    { keycode::num3, '3' },
    { keycode::num4, '4' },
    { keycode::num5, '5' },
    { keycode::num6, '6' },
    { keycode::num7, '7' },
    { keycode::num8, '8' },
    { keycode::num9, '9' },
};


void input_win32::refresh() {
    std::memcpy(_last_key_state, _key_state, sizeof(_key_state));
    GetKeyboardState(_key_state);


    std::memcpy(_last_mouse_state, _mouse_state, sizeof(_mouse_state));
    _mouse_state[static_cast<std::size_t>(mouse_button::left)] = static_cast<std::uint8_t>(GetKeyState(VK_LBUTTON));
    _mouse_state[static_cast<std::size_t>(mouse_button::middle)] = static_cast<std::uint8_t>(GetKeyState(VK_MBUTTON));
    _mouse_state[static_cast<std::size_t>(mouse_button::right)] = static_cast<std::uint8_t>(GetKeyState(VK_RBUTTON));
}

bool input_win32::is_key_down(keycode key) {
    const auto index = key_map[key];
    return _key_state[index] & 0x80;
}

bool input_win32::is_key_up(keycode key) {
    const auto index = key_map[key];
    return (_key_state[index] & 0x80) == 0;
}

bool input_win32::is_key_pressed(keycode key) {
    const auto index = key_map[key];
    return (_key_state[index] & 0x80) && ((_last_key_state[index] & 0x80) == 0);
}

bool input_win32::is_key_released(keycode key) {
    const auto index = key_map[key];
    return ((_key_state[index] & 0x80) == 0) && (_last_key_state[index] & 0x80);
}

vec2i input_win32::mouse_position() {
    POINT position;
    if (GetCursorPos(&position) && ScreenToClient(window::native_handle(), &position)) {
        return { position.x, position.y };
    }
    return { 0, 0 };
}

float input_win32::mouse_wheel() {
    return 0.0f;
}

bool input_win32::is_mouse_button_down(mouse_button button) {
    return _mouse_state[static_cast<std::size_t>(button)] & 0x80;
}

bool input_win32::is_mouse_button_up(mouse_button button) {
    return (_mouse_state[static_cast<std::size_t>(button)] & 0x80) == 0;
}

bool input_win32::is_mouse_button_pressed(mouse_button button) {
    return (_mouse_state[static_cast<std::size_t>(button)] & 0x80) &&
        ((_last_mouse_state[static_cast<std::size_t>(button)] & 0x80) == 0);
}

bool input_win32::is_mouse_button_released(mouse_button button) {
    return ((_mouse_state[static_cast<std::size_t>(button)] & 0x80) == 0) &&
        (_last_mouse_state[static_cast<std::size_t>(button)] & 0x80);
}
