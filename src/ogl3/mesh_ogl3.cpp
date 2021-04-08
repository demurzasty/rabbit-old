#include "mesh_ogl3.hpp"
#include "buffer_ogl3.hpp"

#include <rabbit/exception.hpp>

using namespace rb;

mesh_ogl3::mesh_ogl3(const mesh_desc& desc)
    : mesh(desc) {
    if (!desc.vertex_buffer) {
        throw make_exception("Mesh need atleast vertex buffer to working");
    }

    if (desc.vertex_desc.empty()) {
        throw make_exception("Vertex description is missing");
    }

    auto vertex_buffer = std::static_pointer_cast<buffer_ogl3>(desc.vertex_buffer);
    auto index_buffer = std::static_pointer_cast<buffer_ogl3>(desc.index_buffer);

    std::size_t stride{ 0 };
    for (auto& element : desc.vertex_desc) {
        stride += element.format.size;
    }

    glCreateVertexArrays(1, &_id);

    glVertexArrayVertexBuffer(_id, 0, vertex_buffer->id(), 0, stride);
    // glVertexArrayElementBuffer(_id, index_buffer->id());

    std::size_t index{ 0 };
    std::size_t offset{ 0 };
    for (auto& element : desc.vertex_desc) {
        glEnableVertexArrayAttrib(_id, index);
        glVertexArrayAttribFormat(_id, index,
            element.format.components,
            element.format.type == vertex_format_type::floating_point ? GL_FLOAT : GL_INT,
            element.format.normalize ? GL_TRUE : GL_FALSE,
            offset);
        glVertexArrayAttribBinding(_id, index, 0);
    }
}

mesh_ogl3::~mesh_ogl3() {
    glDeleteVertexArrays(1, &_id);
}

GLuint mesh_ogl3::id() const {
    return _id;
}
