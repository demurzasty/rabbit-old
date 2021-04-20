#include <rabbit/texture.hpp>

using namespace rb;

texture::texture(const texture_desc& desc)
    : _size(desc.size)
    , _format(desc.format) {
    RB_ASSERT(_size.x > 0 && _size.y > 0, "Size of texture must be greater than 0");
}

const vec2u& texture::size() const RB_NOEXCEPT {
    return _size;
}

vec2f texture::texel() const RB_NOEXCEPT {
    return { 1.0f / _size.x, 1.0f / _size.y };
}

texture_format texture::format() const RB_NOEXCEPT {
    return _format;
}


std::size_t texture::bytes_per_pixel() const RB_NOEXCEPT {
    switch (_format) {
        case texture_format::r8: return 1;
        case texture_format::rg8: return 2;
        case texture_format::rgba8: return 4;
        case texture_format::d24s8: return 4;
    }

    RB_ASSERT(false, "Incorrect texture format");
    return 0;
}
