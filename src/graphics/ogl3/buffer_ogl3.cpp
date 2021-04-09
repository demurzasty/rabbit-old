#include "buffer_ogl3.hpp"

#include <rabbit/core/exception.hpp>

#include <map>

using namespace rb;

static std::map<buffer_type, GLenum> types = {
    { buffer_type::vertex, GL_ARRAY_BUFFER },
    { buffer_type::index, GL_ELEMENT_ARRAY_BUFFER },
    { buffer_type::uniform, GL_UNIFORM_BUFFER }
};

buffer_ogl3::buffer_ogl3(const buffer_desc& desc)
    : buffer(desc)
    , _type(types.at(desc.type)) {
    glCreateBuffers(1, &_id);

    constexpr GLbitfield mutable_flags = GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_DYNAMIC_STORAGE_BIT;

    glNamedBufferStorage(_id, desc.size, desc.data, desc.is_mutable ? mutable_flags : 0);
}

buffer_ogl3::~buffer_ogl3() {
    glDeleteBuffers(1, &_id);
}

GLuint buffer_ogl3::id() const {
    return _id;
}

void* buffer_ogl3::map() {
    if (!is_mutable()) {
        throw make_exception("[OGL3] Buffer need to be mutable to update content");
    }

    return glMapNamedBuffer(_id, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
}

void buffer_ogl3::unmap() {
    glUnmapNamedBuffer(_id);
}

void buffer_ogl3::update(const void* data, std::size_t size) {
    if (!is_mutable()) {
        throw make_exception("[OGL3] Buffer need to be mutable to update content");
    }

    if (size > this->size()) {
        throw make_exception("[OGL3] Overflow data");
    }

    glNamedBufferSubData(_id, 0, size, data);
}
