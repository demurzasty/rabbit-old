#include <rabbit/image.hpp>
#include <rabbit/config.hpp>

#define STBI_MAX_DIMENSIONS 8192
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

using namespace rb;

image image::load_from_file(const std::string& filename) {
    int width, height, components;
    std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels{
        stbi_load(filename.c_str(), &width, &height, &components, STBI_rgb_alpha),
        &stbi_image_free
    };
    
    if (!pixels) {
        return {};
    }

    return { reinterpret_cast<const color*>(pixels.get()), vec2u{ static_cast<unsigned int>(width), static_cast<unsigned int>(height) } };
}

image image::load_from_stream(ibstream& stream) {
    stbi_io_callbacks callbacks;
    callbacks.read = [](void* userdata, char* data, int size) -> int {
        auto stream = static_cast<ibstream*>(userdata);
        stream->read(data, size);
        return size;
    };
    callbacks.skip = [](void* userdata, int offset) {
        auto stream = static_cast<ibstream*>(userdata);
        stream->seek(offset);
    };
    callbacks.eof = [](void* userdata) -> int {
        auto stream = static_cast<ibstream*>(userdata);
        return stream->eof() ? 1 : 0;
    };

    int width, height, components;
    std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels{
        stbi_load_from_callbacks(&callbacks, &stream, &width, &height, &components, STBI_rgb_alpha),
        &stbi_image_free
    };
    
    if (!pixels) {
        return {};
    }

    return { reinterpret_cast<const color*>(pixels.get()), vec2u{ static_cast<unsigned int>(width), static_cast<unsigned int>(height) } };
}

image image::resize(const image& image, const vec2u& new_size) {
    const auto pixels = std::make_unique<color[]>(new_size.x * new_size.y);
    const auto result = stbir_resize_uint8(reinterpret_cast<const unsigned char*>(image.pixels().data()), image.size().x, image.size().y, image.stride(), 
        reinterpret_cast<unsigned char*>(pixels.get()), new_size.x, new_size.y, new_size.x * 4, 4);

    RB_ASSERT(result, "Cannot resize image from: ({}, {}) to: ({}, {}).", image.size().x, image.size().y, new_size.x, new_size.y);
    return { pixels.get(), new_size };
}

image::operator bool() const {
    return !_pixels.empty();
}

span<const color> image::pixels() const {
    return _pixels;
}

const vec2u& image::size() const {
    return _size;
}

std::size_t image::stride() const {
    return _size.x * 4;
}

image::image(const color* pixels, const vec2u& size)
    : _pixels(pixels, pixels + size.x * size.y)
    , _size(size) {
}
