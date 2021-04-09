#include <rabbit/graphics/image.hpp>
#include <rabbit/core/enum.hpp>
#include <rabbit/core/exception.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <map>
#include <cassert>

using namespace rb;

static std::map<image_format, std::uint8_t> bytes_per_pixels = {
    { image_format::r8, 1 },
    { image_format::rg8, 2 },
    { image_format::rgb8, 3 },
    { image_format::rgba8, 4 },
};

image image::from_file(const std::string& filename) {
    vec2i size;
    int components;
    auto pixels = storage{ stbi_load(filename.c_str(), &size.x, &size.y, &components, 4), &stbi_image_free };
    if (!pixels) {
        throw make_exception(stbi_failure_reason());
    }

    return { std::move(pixels), size, image_format::rgba8 };
}

image::image(storage&& pixels, const vec2i& size, image_format format)
    : _pixels(std::forward<storage>(pixels)), _size(size), _format(format) {
}

span<const std::uint8_t> image::pixels() const {
    return { _pixels.get(), static_cast<std::size_t>(_size.x) * _size.y * bytes_per_pixels.at(_format) };
}

const vec2i& image::size() const {
    return _size;
}

image_format image::format() const {
    return _format;
}
