#include <rabbit/graphics/vertex.hpp>
#include <rabbit/core/range.hpp>

using namespace rb;

vertex_desc::vertex_desc(std::initializer_list<vertex_element> elements)
    : _elements(elements) {
    _elements.shrink_to_fit();
}

const vertex_element& vertex_desc::operator[](std::size_t index) const {
    RB_ASSERT(index < size(), "Out of bound");
    return _elements[index];
}

vertex_desc::iterator vertex_desc::begin() RB_NOEXCEPT {
    return _elements.begin();
}

vertex_desc::const_iterator vertex_desc::begin() const RB_NOEXCEPT {
    return _elements.begin();
}

vertex_desc::iterator vertex_desc::end() RB_NOEXCEPT {
    return _elements.end();
}

vertex_desc::const_iterator vertex_desc::end() const RB_NOEXCEPT {
    return _elements.end();
}

bool vertex_desc::empty() const RB_NOEXCEPT {
    return _elements.empty();
}

std::size_t vertex_desc::size() const RB_NOEXCEPT {
    return _elements.size();
}

std::size_t vertex_desc::stride() const RB_NOEXCEPT {
    // todo: cache
    std::size_t stride{ 0 };
    for (auto& element : _elements) {
        stride += element.format.size;
    }
    return stride;
}

std::size_t vertex_desc::offset(std::size_t vertex_index) const RB_NOEXCEPT {
    // todo: cache
    std::size_t offset{ 0 };
    for (auto index : rb::make_range<std::size_t>(0u, vertex_index)) {
        offset += _elements[index].format.size;
    }
    return offset;
}
