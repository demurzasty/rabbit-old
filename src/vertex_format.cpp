#include <rabbit/vertex_format.hpp>
#include <rabbit/vec2.hpp>
#include <rabbit/vec3.hpp>
#include <rabbit/vec4.hpp>

#include <map>

using namespace rb;

namespace {
    std::map<vertex_format, std::size_t> formats = {
        { vertex_format::vec2f, sizeof(vec2f) },
        { vertex_format::vec3f, sizeof(vec3f) },
        { vertex_format::vec4f, sizeof(vec4f) },
        { vertex_format::vec2i, sizeof(vec2i) },
        { vertex_format::vec3i, sizeof(vec3i) },
        { vertex_format::vec4i, sizeof(vec4i) }
    };
}

[[nodiscard]] std::size_t rb::vertex_format_size(const vertex_format format) {
    return formats.at(format);
}
