#include "main.hpp"

hello_world::hello_world(rb::config& config)
    : rb::game(config) {
}

void hello_world::initialize() {
}

void hello_world::update(float elapsed_time) {
    if (gamepad()->is_button_pressed(rb::gamepad_player::first, rb::gamepad_button::back) ||
        keyboard()->is_key_pressed(rb::keycode::escape)) {
        exit();
    }
}

void hello_world::draw() {
    graphics_device()->clear(rb::color::cornflower_blue());
}

int main(int argc, char* argv[]) {
    rb::config config;
    config.window_title = "Hello World";
    config.window_size = { 1280, 720 };
    hello_world{ config }.run();
}
