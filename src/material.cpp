#include <rabbit/material.hpp>

using namespace rb;

material::material(const material_desc& desc)
    : _vertex_desc(desc.vertex_desc)
    , _bindings(desc.bindings) {
    RB_ASSERT(!_vertex_desc.empty(), "Vertex description is not provided");
}

const vertex_desc& material::vertex_desc() const RB_NOEXCEPT {
    return _vertex_desc;
}

const std::vector<material_binding_desc>& material::bindings() const RB_NOEXCEPT {
    return _bindings;
}
