#include "buffer_ogl3.hpp"

#include <rabbit/exception.hpp>

#include <map>

using namespace rb;

static std::map<buffer_type, GLenum> types = {
    { buffer_type::vertex, GL_ARRAY_BUFFER },
    { buffer_type::index, GL_ELEMENT_ARRAY_BUFFER }
};

buffer_ogl3::buffer_ogl3(const buffer_desc& desc)
    : buffer(desc)
    , _type(types.at(desc.type)) {
    glGenBuffers(1, &_id);
    glBindBuffer(_type, _id);

    if (desc.data) {
        glBufferData(_type, desc.size, desc.data, desc.is_mutable ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    } else {
        glBufferData(_type, desc.size, 0, desc.is_mutable ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    }

    glBindBuffer(_type, 0);
}

buffer_ogl3::~buffer_ogl3() {
    glDeleteBuffers(1, &_id);
}

GLuint buffer_ogl3::id() const {
    return _id;
}

void* buffer_ogl3::map() {
    if (!is_mutable()) {
        throw exception{ "[OGL3] Buffer need to be mutable to update content" };
    }

    glBindBuffer(_type, _id);
    return glMapBuffer(_type, GL_READ_WRITE);
}

void buffer_ogl3::unmap() {
    glUnmapBuffer(_type);
}

void buffer_ogl3::update(const void* data, std::size_t size) {
    if (!is_mutable()) {
        throw exception{ "[OGL3] Buffer need to be mutable to update content" };
    }

    if (size > this->size()) {
        throw exception{ "[OGL3] Overflow data" };
    }

    glBindBuffer(_type, _id);
    glBufferSubData(_type, 0, size, data);
}
