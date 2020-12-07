#include "keyboard_glfw.hpp"
#include "window_glfw.hpp"

#include <string.h>
#include <unordered_map>

using namespace rb;

static std::unordered_map<keycode, int> keys = {
    { keycode::left, GLFW_KEY_LEFT },
    { keycode::right, GLFW_KEY_RIGHT },
    { keycode::up, GLFW_KEY_UP },
    { keycode::down, GLFW_KEY_DOWN }
};

keyboard_glfw::keyboard_glfw(std::shared_ptr<window> window)
    : _window(window)
    , _last_state()
    , _state() {
}

void keyboard_glfw::refresh() {
    memcpy(_last_state, _state, sizeof(_last_state));
    
    auto native_window = std::static_pointer_cast<window_glfw>(_window)->window();
    for (const auto [keycode, key] : keys) {
        _state[key] = glfwGetKey(native_window, key);
    }
}

bool keyboard_glfw::is_key_down(keycode key) {
    return _state[keys[key]];
}

bool keyboard_glfw::is_key_up(keycode key) {
    return !_state[keys[key]];
}

bool keyboard_glfw::is_key_pressed(keycode key) {
    const auto index = keys[key];
    return _state[index] && !_last_state[index];
}

bool keyboard_glfw::is_key_released(keycode key) {
    const auto index = keys[key];
    return !_state[index] && _last_state[index];
}
