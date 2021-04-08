#include <rabbit/mesh.hpp>

using namespace rb;

mesh::mesh(const mesh_desc& desc)
    : _topology(desc.topology)
    , _vertex_desc(desc.vertex_desc)
    , _vertex_buffer(desc.vertex_buffer)
    , _index_buffer(desc.index_buffer) {
}

topology mesh::topology() const {
    return _topology;
}

const vertex_desc& mesh::vertex_desc() {
    return _vertex_desc;
}

const std::shared_ptr<buffer>& mesh::vertex_buffer() {
    return _vertex_buffer;
}

const std::shared_ptr<buffer>& mesh::index_buffer() {
    return _index_buffer;
}
