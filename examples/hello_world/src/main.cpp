#include "main.hpp"

hello_world::hello_world(rb::config& config)
    : rb::game(config) {
}

void hello_world::initialize() {
}

void hello_world::update(float elapsed_time) {
    game::update(elapsed_time);

    // Exit game if back button on gamepad or escape on keyboard was pressed.
    if (gamepad()->is_button_pressed(rb::gamepad_player::first, rb::gamepad_button::back) ||
        keyboard()->is_key_pressed(rb::keycode::escape)) {
        exit();
    }
}

void hello_world::draw() {
    // Clear window with color.
    graphics_device()->clear(rb::color::cornflower_blue());

    game::draw();
}

int main(int argc, char* argv[]) {
    rb::config config;
    config.window_title = "Hello World";
    config.window_size = { 960, 640 };
    hello_world{ config }.run();
}
