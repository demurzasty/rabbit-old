#include <rabbit/mesh.hpp>

using namespace rb;

mesh::mesh(const mesh_desc& desc)
    : _vertex_desc(desc.vertex_desc)
    , _vertex_buffer(desc.vertex_buffer)
    , _index_type(desc.index_type)
    , _index_buffer(desc.index_buffer) {
    RB_ASSERT(!_vertex_desc.empty(), "Vertex description is not provided");
    RB_ASSERT(_vertex_buffer, "Vertex buffer is not provided");
}

const vertex_desc& mesh::vertex_desc() const RB_NOEXCEPT {
    return _vertex_desc;   
}

const std::shared_ptr<buffer>& mesh::vertex_buffer() const RB_NOEXCEPT {
    return _vertex_buffer;
}

index_type mesh::index_type() const RB_NOEXCEPT {
    return _index_type;
}

const std::shared_ptr<buffer>& mesh::index_buffer() const RB_NOEXCEPT {
    return _index_buffer;
}
