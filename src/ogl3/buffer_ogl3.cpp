#include "buffer_ogl3.hpp"

#include <map>

using namespace rb;

static std::map<buffer_type, GLenum> buffers = {
    { buffer_type::vertex, GL_ARRAY_BUFFER },
    { buffer_type::index, GL_ELEMENT_ARRAY_BUFFER }
};

buffer_ogl3::buffer_ogl3(const buffer_desc& desc)
    : buffer(desc) {
    glGenBuffers(1, &_id);
    glBindBuffer(buffers.at(desc.type), _id);

    if (desc.data) {
        glBufferData(buffers.at(desc.type), desc.size, desc.data, desc.is_mutable ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    }

    glBindBuffer(buffers.at(desc.type), 0);
}

buffer_ogl3::~buffer_ogl3() {
    glDeleteBuffers(1, &_id);
}

GLuint buffer_ogl3::id() const {
    return _id;
}
