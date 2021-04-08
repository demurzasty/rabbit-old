#include <rabbit/buffer.hpp>
#include <rabbit/exception.hpp>

using namespace rb;

buffer::buffer(const buffer_desc& desc)
    : _type(desc.type)
    , _size(desc.size)
    , _stride(desc.stride)
    , _is_mutable(desc.is_mutable) {
    if (desc.size == 0) {
        throw make_exception("Buffer size cannot be 0");
    }
    if (desc.stride == 0) {
        throw make_exception("Buffer stride cannot be 0");
    }
}

buffer_type buffer::type() const {
    return _type;
}

std::size_t buffer::size() const {
    return _size;
}

std::size_t buffer::stride() const {
    return _stride;
}

std::size_t buffer::count() const {
    return size() / stride();
}

bool buffer::is_mutable() const {
    return _is_mutable;
}
