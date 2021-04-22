#include <rabbit/graphics/buffer.hpp>

using namespace rb;

buffer::buffer(const buffer_desc& desc)
    : _type(desc.type)
    , _size(desc.size)
    , _stride(desc.stride) {
    RB_ASSERT(!desc.data || _size > 0, "Size of buffer must be not 0 if data was provided");
    RB_ASSERT(_stride > 0, "Stride must be not 0");
}

buffer_type buffer::type() const RB_NOEXCEPT {
    return _type;
}

std::size_t buffer::size() const RB_NOEXCEPT {
    return _size;
}

std::size_t buffer::stride() const RB_NOEXCEPT {
    return _stride;
}

std::size_t buffer::count() const RB_NOEXCEPT {
    return _size / _stride;
}
