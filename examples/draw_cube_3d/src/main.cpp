#include "main.hpp"

#include <rabbit/vertex.hpp>

example_game::example_game(rb::config& config)
    : rb::game(config) {
}

void example_game::initialize() {
    game::initialize();

    // Load texture from file.
    _texture = asset_manager()->load<rb::texture>("data/texture.png");

    rb::vertex vertices[24] = {
        { { -1.0f, 1.0f, -1.0f }, { 0.0f, 0.0f }, rb::color::white() },
        { { 1.0f, 1.0f, -1.0f }, { 1.0f, 0.0f }, rb::color::white() },
        { { -1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f }, rb::color::white() },
        { { 1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f }, rb::color::white() },

        { { -1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }, rb::color::white() },
        { { -1.0f, 1.0f, -1.0f }, { 1.0f, 0.0f }, rb::color::white() },
        { { -1.0f, -1.0f, 1.0f }, { 0.0f, 1.0f }, rb::color::white() },
        { { -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f }, rb::color::white() },

        { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }, rb::color::white() },
        { { -1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }, rb::color::white() },
        { { 1.0f, -1.0f, 1.0f }, { 0.0f, 1.0f }, rb::color::white() },
        { { -1.0f, -1.0f, 1.0f }, { 1.0f, 1.0f }, rb::color::white() },

        { { 1.0f, 1.0f, -1.0f }, { 0.0f, 0.0f }, rb::color::white() },
        { { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }, rb::color::white() },
        { { 1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f }, rb::color::white() },
        { { 1.0f, -1.0f, 1.0f }, { 1.0f, 1.0f }, rb::color::white() },

        { { -1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }, rb::color::white() },
        { { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }, rb::color::white() },
        { { -1.0f, 1.0f, -1.0f }, { 0.0f, 1.0f }, rb::color::white() },
        { { 1.0f, 1.0f, -1.0f }, { 1.0f, 1.0f }, rb::color::white() },

        { { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f }, rb::color::white() },
        { { 1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f }, rb::color::white() },
        { { -1.0f, -1.0f, 1.0f }, { 0.0f, 1.0f }, rb::color::white() },
        { { 1.0f, -1.0f, 1.0f }, { 1.0f, 1.0f }, rb::color::white() }
    };

    std::uint32_t indices[36] = { 
        0, 1, 2, 
        2, 3, 1,
    
        4, 5, 6,
        6, 7, 5,

        8, 9, 10,
        10, 11, 9,

        12, 13, 14,
        14, 15, 13,

        16, 17, 18,
        18, 19, 17,

        20, 21, 22,
        22, 23, 21
    };

    rb::buffer_desc desc;
    desc.type = rb::buffer_type::vertex;
    desc.size = sizeof(vertices);
    desc.data = vertices;
    desc.is_mutable = false;
    _vertex_buffer = graphics_device()->make_buffer(desc);

    desc.type = rb::buffer_type::index;
    desc.size = sizeof(indices);
    desc.data = indices;
    desc.is_mutable = false;
    _index_buffer = graphics_device()->make_buffer(desc);

    graphics_device()->set_depth_test(true);
}

void example_game::update(float elapsed_time) {
    if (gamepad()->is_button_pressed(rb::gamepad_player::first, rb::gamepad_button::back) ||
        keyboard()->is_key_pressed(rb::keycode::escape)) {
        exit();
    }

    _rotation += elapsed_time;
}

void example_game::draw() {
    graphics_device()->clear(rb::color::cornflower_blue());

    graphics_device()->set_projection_matrix(rb::mat4f::perspective(45.0f, 16.0f / 9.0f, 0.1f, 10.0f));
    graphics_device()->set_view_matrix(rb::mat4f::translation({ 0.0f, 0.0f, -5.0f }));
    graphics_device()->set_world_matrix(rb::mat4f::rotation({ _rotation }));
   
    graphics_device()->draw_textured(rb::topology::triangles, _vertex_buffer, _index_buffer, _texture);
}

int main(int argc, char* argv[]) {
    rb::config config;
    config.window.title = "Draw Cube 3D Example";
    config.window.size = { 1280, 720 };
    example_game{ config }.run();
}
