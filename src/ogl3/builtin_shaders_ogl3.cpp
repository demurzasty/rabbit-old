#include <rabbit/builtin_shaders.hpp>

#include <shaders/forward.vert.glsl.h>
#include <shaders/forward.frag.glsl.h>

#include <map>

using namespace rb;

span<const std::uint8_t> builtin_shaders::get(builtin_shader shader) {
    switch (shader) {
        case builtin_shader::forward_vert:
            return forward_vert_glsl;
        case builtin_shader::forward_frag:
            return forward_frag_glsl;
    }

    return {};
}
