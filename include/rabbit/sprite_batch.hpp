#pragma once 

#include <rabbit/graphics_device.hpp>

#include <vector>

namespace rb {
    class sprite_batch {
    public:
        sprite_batch(std::shared_ptr<graphics_device> graphics_device);

        void begin();

        void draw(std::shared_ptr<texture> texture, const vec4i& source, const vec4f& destination, const color& color);

        void end();

    private:
        void flush();

    private:
        std::shared_ptr<graphics_device> _graphics_device;
        std::vector<vertex> _vertices;
        std::shared_ptr<texture> _current_texture;
    };
}
