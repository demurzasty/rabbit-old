#include "mouse_dummy.hpp"

using namespace rb;

void mouse_dummy::refresh() {
}

vec2i mouse_dummy::position() {
    return { 0, 0 };
}

float mouse_dummy::wheel() {
    return 0.0f;
}

bool mouse_dummy::is_button_down(mouse_button button) {
    return false;
}

bool mouse_dummy::is_button_up(mouse_button button) {
    return true;
}

bool mouse_dummy::is_button_pressed(mouse_button button) {
    return false;
}

bool mouse_dummy::is_button_released(mouse_button button) {
    return false;
}
