#include "main.hpp"
#include "test_state.hpp"

example_game::example_game(rb::config& config)
    : rb::game(config) {
}

void example_game::initialize() {
    game::initialize();

    state_manager()->add("test_state", std::make_shared<test_state>());

    state_manager()->set("test_state");
}

void example_game::update(float elapsed_time) {
    // Exit game if back button on gamepad or escape on keyboard was pressed.
    if (gamepad()->is_button_pressed(rb::gamepad_player::first, rb::gamepad_button::back) ||
        keyboard()->is_key_pressed(rb::keycode::escape)) {
        exit();
    }
}

void example_game::draw() {
    // Clear window with color.
    graphics_device()->clear(rb::color::cornflower_blue());
}

int main(int argc, char* argv[]) {
    rb::config config;
    config.window_title = "State Manager Example";
    config.window_size = { 960, 640 };
    example_game{ config }.run();
}
