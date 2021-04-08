#include <rabbit/shader.hpp>

using namespace rb;

shader::shader(const shader_desc& desc)
    : _vertex_desc(desc.vertex_desc) {
}

const vertex_desc& shader::vertex_desc() const {
    return _vertex_desc;
}
