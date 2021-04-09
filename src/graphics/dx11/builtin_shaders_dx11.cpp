#include <rabbit/builtin_shaders.hpp>

#include <shaders/forward.vert.comp.h>
#include <shaders/forward.frag.comp.h>

using namespace rb;

span<const std::uint8_t> builtin_shaders::get(builtin_shader shader) {
    switch (shader) {
        case builtin_shader::forward_vert:
            return forward_vert_comp;
        case builtin_shader::forward_frag:
            return forward_frag_comp;
    }

    return {};
}
