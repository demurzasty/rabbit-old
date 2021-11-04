#include <rabbit/image.hpp>

#define STBI_MAX_DIMENSIONS 8192
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
