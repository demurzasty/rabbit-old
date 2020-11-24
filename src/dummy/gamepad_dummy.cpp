#include "gamepad_dummy.hpp"

using namespace rb;

void gamepad_dummy::refresh() {
}

bool gamepad_dummy::is_button_down(gamepad_player player, gamepad_button button) {
    return false;
}

bool gamepad_dummy::is_button_up(gamepad_player player, gamepad_button button) {
    return true;
}

bool gamepad_dummy::is_button_pressed(gamepad_player player, gamepad_button button) {
    return false;
}

bool gamepad_dummy::is_button_released(gamepad_player player, gamepad_button button) {
    return false;
}

float gamepad_dummy::axis(gamepad_player player, gamepad_axis axis) {
    return 0.0f;
}
