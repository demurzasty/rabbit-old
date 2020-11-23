#include "main.hpp"

// Texture used: https://opengameart.org/content/orthographic-outdoor-tiles

example_game::example_game(rb::config& config)
    : rb::game(config) {
}

void example_game::initialize() {
    game::initialize();

    _sprite_batch = std::make_shared<rb::sprite_batch>(graphics_device());

    // Load texture from file.
    _texture = asset_manager()->load<rb::texture>("data/mockup.png");
}

void example_game::update(float elapsed_time) {
    if (gamepad()->is_button_pressed(rb::gamepad_player::first, rb::gamepad_button::back) ||
        keyboard()->is_key_pressed(rb::keycode::escape)) {
        exit();
    }
}

void example_game::draw() {
    graphics_device()->clear(rb::color::cornflower_blue());

    _sprite_batch->begin();

    _sprite_batch->draw(_texture, { 0, 0, 480, 320 }, { 0.0f, 0.0f, 960.0f, 640.0f }, rb::color::white());

    _sprite_batch->end();
}

int main(int argc, char* argv[]) {
    rb::config config;
    config.window_title = "Draw Texture Example";
    config.window_size = { 960, 640 };
    example_game{ config }.run();
}
