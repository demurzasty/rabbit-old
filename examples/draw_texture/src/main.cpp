#include "main.hpp"

// Texture used: https://opengameart.org/content/orthographic-outdoor-tiles

draw_texture::draw_texture(rb::config& config)
    : rb::game(config) {
}

void draw_texture::initialize() {
    // Load texture from file.
    _texture = asset_manager()->load<rb::texture>("data/mockup.png");
}

void draw_texture::update(float elapsed_time) {
    if (gamepad()->is_button_pressed(rb::gamepad_player::first, rb::gamepad_button::back) ||
        keyboard()->is_key_pressed(rb::keycode::escape)) {
        exit();
    }
}

void draw_texture::draw() {
    graphics_device()->clear(rb::color::cornflower_blue());

    graphics_device()->set_projection_matrix(rb::mat4f::orthographic(0.0f, 480.0f, 320.0f, 0.0f, -1.0f, 1.0f));

    rb::vertex2d vertices[4];
    vertices[0] = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, rb::color::white() };
    vertices[1] = { { 480.0f, 0.0f }, { 1.0f, 0.0f }, rb::color::white() };
    vertices[2] = { { 0.0f, 320.0f }, { 0.0f, 1.0f }, rb::color::white() };
    vertices[3] = { { 480.0f, 320.0f }, { 1.0f, 1.0f }, rb::color::white() };
    graphics_device()->draw_textured(rb::topology::triangle_strip, vertices, _texture);
}

int main(int argc, char* argv[]) {
    rb::config config;
    config.window_title = "Hello World";
    config.window_size = { 960, 640 };
    draw_texture{ config }.run();
}
