#include "main.hpp"

// Texture used: https://opengameart.org/content/micro-character-bases-basics

example_game::example_game(rb::config& config)
    : rb::game(config) {
}

void example_game::initialize() {
    game::initialize();

    _sprite_batch = std::make_shared<rb::sprite_batch>(graphics_device());

    // Load texture from file.
    _texture = asset_manager()->load<rb::texture>("data/troll_nude_hair.png");
}

void example_game::update(float elapsed_time) {
    if (gamepad()->is_button_pressed(rb::gamepad_player::first, rb::gamepad_button::back) ||
        keyboard()->is_key_pressed(rb::keycode::escape)) {
        exit();
    }

    auto horizontal = gamepad()->axis(rb::gamepad_player::first, rb::gamepad_axis::left_x);
    auto vertical = gamepad()->axis(rb::gamepad_player::first, rb::gamepad_axis::left_y);

    _troll_position = _troll_position + rb::vec2f{ horizontal, vertical } *elapsed_time * 60.0f;
}

void example_game::draw() {
    graphics_device()->clear(rb::color::cornflower_blue());

    _sprite_batch->begin();

    _sprite_batch->draw(_texture, { 0, 20, 20, 20 }, { _troll_position.x, _troll_position.y, 80.0f, 80.0f }, rb::color::white());

    _sprite_batch->end();
}

int main(int argc, char* argv[]) {
    rb::config config;
    config.window.title = "Gamepad Example";
    config.window.size = { 1280, 720 };
    example_game{ config }.run();
}
