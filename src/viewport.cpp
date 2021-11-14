#include <rabbit/viewport.hpp>
#include <rabbit/config.hpp>

using namespace rb;

const vec2u& viewport::size() const {
    return _size;
}

float viewport::aspect() const {
    return _size.x / static_cast<float>(_size.y);
}

viewport::viewport(const viewport_desc& desc)
    : _size(desc.size) {
    RB_ASSERT(_size.x > 0 && _size.y > 0, "Size of viewport should be greater than 0. Current size: {}, {}.", _size.x, _size.y);
}
