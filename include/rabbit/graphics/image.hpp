#pragma once 

#include "../core/span.hpp"
#include "../math/vec2.hpp"
#include "image_format.hpp"

#include <string>
#include <memory>
#include <cstdint>

namespace rb {
    class image {
    public:
        using storage = std::unique_ptr<std::uint8_t, void(*)(void*)>;

    public:
        static image from_file(const std::string& filename);

    public:
        image(storage&& pixels, const vec2i& size, image_format format);

        image(const image&) = delete;
        image(image&&) = default;

        image& operator=(const image&) = delete;
        image& operator=(image&&) = default;

        span<const std::uint8_t> pixels() const;

        const vec2i& size() const;

        image_format format() const;

    private:
        storage _pixels;
        vec2i _size;
        image_format _format;
    };
}
