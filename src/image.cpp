#include <rabbit/image.hpp>
#include <rabbit/enum.hpp>
#include <rabbit/exception.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <map>
#include <cassert>

using namespace rb;

static std::size_t bytes_per_pixels[] = {
    1,
    2,
    3,
    4
};

image image::from_file(const std::string& filename) {
    vec2i size;
    int components;
    auto pixels = storage{ stbi_load(filename.c_str(), &size.x, &size.y, &components, 4), &stbi_image_free };
    if (!pixels) {
        throw exception{ stbi_failure_reason() };
    }

    return { std::move(pixels), size, image_format::rgba8 };
}

image::image(storage&& pixels, const vec2i& size, image_format format)
    : _pixels(std::forward<storage>(pixels)), _size(size), _format(format) {
}

span<const std::uint8_t> image::pixels() const {
    return { _pixels.get(), static_cast<std::size_t>(_size.x) * _size.y * bytes_per_pixels[enum_size(_format)] };
}

const vec2i& image::size() const {
    return _size;
}

image_format image::format() const {
    return _format;
}
