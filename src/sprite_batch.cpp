#include <rabbit/sprite_batch.hpp>

#include <cassert>

using namespace rb;

sprite_batch::sprite_batch(std::shared_ptr<graphics_device> graphics_device)
    : _graphics_device(graphics_device) {
    assert(_graphics_device);
}

void sprite_batch::begin(const vec2i& canvas_size) {
    assert(_vertices.empty());
    assert(!_current_texture);

    const auto backbuffer_size = static_cast<vec2f>(canvas_size);
    begin(rb::mat4f::orthographic(0.0f, backbuffer_size.x, backbuffer_size.y, 0.0f, -1.0f, 1.0f), rb::mat4f::identity());
}

void sprite_batch::begin(const mat4f& projection_matrix, const mat4f& view_matrix) {
    _graphics_device->set_projection_matrix(projection_matrix);
    _graphics_device->set_view_matrix(view_matrix);
    _graphics_device->set_world_matrix(rb::mat4f::identity());
}

void sprite_batch::draw(std::shared_ptr<texture> texture, const vec4i& source, const vec4f& destination, const color& color) {
    assert(texture);

    if (_current_texture && _current_texture != texture) {
        flush();
    }

    const auto texture_size = static_cast<vec2f>(texture->size());

    const vertex vertices[4] = {
        {
            { destination.x, destination.y },
            { source.x / texture_size.x, source.y / texture_size.y },
            color
        },
        {
            { destination.x + destination.z, destination.y },
            { (source.x + source.z) / texture_size.x, source.y / texture_size.y },
            color
        },
        {
            { destination.x + destination.z, destination.y + destination.w },
            { (source.x + source.z) / texture_size.x, (source.y + source.w) / texture_size.y },
            color
        },
        {
            { destination.x, destination.y + destination.w },
            { source.x / texture_size.x, (source.y + source.w) / texture_size.y },
            color
        }
    };

    _vertices.push_back(vertices[0]);
    _vertices.push_back(vertices[1]);
    _vertices.push_back(vertices[2]);
    _vertices.push_back(vertices[2]);
    _vertices.push_back(vertices[3]);
    _vertices.push_back(vertices[0]);

    _current_texture = texture;
}

void sprite_batch::end() {
    flush();
}

void sprite_batch::flush() {
    if (!_vertices.empty() && _current_texture) {
        _graphics_device->draw_textured(topology::triangles, _vertices, _current_texture);
    }

    _vertices.clear();
    _current_texture = nullptr;
}
