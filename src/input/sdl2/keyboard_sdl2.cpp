#include "keyboard_sdl2.hpp"
#include "../../platform/sdl2/window_sdl2.hpp"

#include <unordered_map>

using namespace rb;

static std::unordered_map<keycode, int> keys = {
    { keycode::left_system, SDL_SCANCODE_APPLICATION },
    { keycode::right_system, SDL_SCANCODE_APPLICATION },
    { keycode::menu, SDL_SCANCODE_MENU },
    { keycode::semicolon, SDL_SCANCODE_SEMICOLON },
    { keycode::slash, SDL_SCANCODE_SLASH },
    { keycode::equal, SDL_SCANCODE_EQUALS },
    { keycode::minus, SDL_SCANCODE_MINUS },
    { keycode::left_bracket, SDL_SCANCODE_LEFTBRACKET },
    { keycode::right_bracket, SDL_SCANCODE_RIGHTBRACKET },
    { keycode::comma, SDL_SCANCODE_COMMA },
    { keycode::period, SDL_SCANCODE_PERIOD },
    { keycode::quote, SDL_SCANCODE_APOSTROPHE },
    { keycode::backslash, SDL_SCANCODE_BACKSLASH },
    { keycode::tilde, SDL_SCANCODE_GRAVE },
    { keycode::escape, SDL_SCANCODE_ESCAPE },
    { keycode::space, SDL_SCANCODE_SPACE },
    { keycode::enter, SDL_SCANCODE_RETURN },
    { keycode::backspace, SDL_SCANCODE_BACKSPACE },
    { keycode::tab, SDL_SCANCODE_TAB },
    { keycode::page_up, SDL_SCANCODE_PAGEUP },
    { keycode::page_down, SDL_SCANCODE_PAGEDOWN },
    { keycode::end, SDL_SCANCODE_END },
    { keycode::home, SDL_SCANCODE_HOME },
    { keycode::insert, SDL_SCANCODE_INSERT },
    { keycode::del, SDL_SCANCODE_KP_PERIOD },
    { keycode::add, SDL_SCANCODE_KP_PLUS },
    { keycode::subtract, SDL_SCANCODE_KP_MINUS },
    { keycode::multiply, SDL_SCANCODE_KP_MULTIPLY },
    { keycode::divide, SDL_SCANCODE_KP_DIVIDE },
    { keycode::pause, SDL_SCANCODE_PAUSE },
    { keycode::f1, SDL_SCANCODE_F1 },
    { keycode::f2, SDL_SCANCODE_F2 },
    { keycode::f3, SDL_SCANCODE_F3 },
    { keycode::f4, SDL_SCANCODE_F4 },
    { keycode::f5, SDL_SCANCODE_F5 },
    { keycode::f6, SDL_SCANCODE_F6 },
    { keycode::f7, SDL_SCANCODE_F7 },
    { keycode::f8, SDL_SCANCODE_F8 },
    { keycode::f9, SDL_SCANCODE_F9 },
    { keycode::f10, SDL_SCANCODE_F10 },
    { keycode::f11, SDL_SCANCODE_F11 },
    { keycode::f12, SDL_SCANCODE_F12 },
    { keycode::f13, SDL_SCANCODE_F13 },
    { keycode::f14, SDL_SCANCODE_F14 },
    { keycode::f15, SDL_SCANCODE_F15 },
    { keycode::left, SDL_SCANCODE_LEFT },
    { keycode::right, SDL_SCANCODE_RIGHT },
    { keycode::up, SDL_SCANCODE_UP },
    { keycode::down, SDL_SCANCODE_DOWN },
    { keycode::numpad0, SDL_SCANCODE_KP_0 },
    { keycode::numpad1, SDL_SCANCODE_KP_1 },
    { keycode::numpad2, SDL_SCANCODE_KP_2 },
    { keycode::numpad3, SDL_SCANCODE_KP_3 },
    { keycode::numpad4, SDL_SCANCODE_KP_4 },
    { keycode::numpad5, SDL_SCANCODE_KP_5 },
    { keycode::numpad6, SDL_SCANCODE_KP_6 },
    { keycode::numpad7, SDL_SCANCODE_KP_7 },
    { keycode::numpad8, SDL_SCANCODE_KP_8 },
    { keycode::numpad9, SDL_SCANCODE_KP_9 },
    { keycode::a, SDL_SCANCODE_A },
    { keycode::b, SDL_SCANCODE_B },
    { keycode::c, SDL_SCANCODE_C },
    { keycode::d, SDL_SCANCODE_D },
    { keycode::e, SDL_SCANCODE_E },
    { keycode::f, SDL_SCANCODE_F },
    { keycode::g, SDL_SCANCODE_G },
    { keycode::h, SDL_SCANCODE_H },
    { keycode::i, SDL_SCANCODE_I },
    { keycode::j, SDL_SCANCODE_J },
    { keycode::k, SDL_SCANCODE_K },
    { keycode::l, SDL_SCANCODE_L },
    { keycode::m, SDL_SCANCODE_M },
    { keycode::n, SDL_SCANCODE_N },
    { keycode::o, SDL_SCANCODE_O },
    { keycode::p, SDL_SCANCODE_P },
    { keycode::q, SDL_SCANCODE_Q },
    { keycode::r, SDL_SCANCODE_R },
    { keycode::s, SDL_SCANCODE_S },
    { keycode::t, SDL_SCANCODE_T },
    { keycode::u, SDL_SCANCODE_U },
    { keycode::v, SDL_SCANCODE_V },
    { keycode::w, SDL_SCANCODE_W },
    { keycode::x, SDL_SCANCODE_X },
    { keycode::y, SDL_SCANCODE_Y },
    { keycode::z, SDL_SCANCODE_Z },
    { keycode::num0, SDL_SCANCODE_0 },
    { keycode::num1, SDL_SCANCODE_1 },
    { keycode::num2, SDL_SCANCODE_2 },
    { keycode::num3, SDL_SCANCODE_3 },
    { keycode::num4, SDL_SCANCODE_4 },
    { keycode::num5, SDL_SCANCODE_5 },
    { keycode::num6, SDL_SCANCODE_6 },
    { keycode::num7, SDL_SCANCODE_7 },
    { keycode::num8, SDL_SCANCODE_8 },
    { keycode::num9, SDL_SCANCODE_9 },
};

keyboard_sdl2::keyboard_sdl2(window& window)
    : _window(window)
    , _last_state()
    , _state() {
}

void keyboard_sdl2::refresh() {
    SDL_memcpy(_last_state, _state, sizeof(_last_state));
    SDL_memcpy(_state, SDL_GetKeyboardState(0), sizeof(_state));
}

bool keyboard_sdl2::is_key_down(keycode key) {
    return _state[keys[key]];
}

bool keyboard_sdl2::is_key_up(keycode key) {
    return !_state[keys[key]];
}

bool keyboard_sdl2::is_key_pressed(keycode key) {
    const auto index = keys[key];
    return _state[index] && !_last_state[index];
}

bool keyboard_sdl2::is_key_released(keycode key) {
    const auto index = keys[key];
    return !_state[index] && _last_state[index];
}

const std::string& keyboard_sdl2::input_text() const {
    return static_cast<window_sdl2&>(_window).input_text();
}
