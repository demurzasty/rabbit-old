#include <rabbit/buffer.hpp>
#include <rabbit/exception.hpp>
#include <rabbit/vertex.hpp>

#include <map>

using namespace rb;

static std::map<buffer_type, std::size_t> strides = {
    { buffer_type::vertex, sizeof(vertex) },
    { buffer_type::index, sizeof(std::uint32_t) },
};

buffer::buffer(const buffer_desc& desc)
    : _type(desc.type)
    , _size(desc.size)
    , _is_mutable(desc.is_mutable) {
    if (desc.size == 0) {
        throw exception{ "Buffer size cannot be 0" };
    }
}

buffer_type buffer::type() const {
    return _type;
}

std::size_t buffer::size() const {
    return _size;
}

std::size_t buffer::stride() const {
    return strides.at(_type);
}

std::size_t buffer::count() const {
    return size() / stride();
}

bool buffer::is_mutable() const {
    return _is_mutable;
}
