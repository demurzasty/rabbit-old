#include "keyboard_dummy.hpp"

using namespace rb;

void keyboard_dummy::refresh() {
}

bool keyboard_dummy::is_key_down(keycode key) {
    return false;
}

bool keyboard_dummy::is_key_up(keycode key) {
    return true;
}

bool keyboard_dummy::is_key_pressed(keycode key) {
    return false;
}

bool keyboard_dummy::is_key_released(keycode key) {
    return false;
}

const std::string& keyboard_dummy::input_text() const {
    static std::string text;
    return text;
}
