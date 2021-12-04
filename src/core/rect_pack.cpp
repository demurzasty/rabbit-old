#include <rabbit/core/rect_pack.hpp>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

using namespace rb;

rect_packer::rect_packer(const vec2u& size)
    : _context(std::make_unique<stbrp_context>())
    , _nodes(std::make_unique<stbrp_node[]>(size.x)) {
    stbrp_init_target(_context.get(), static_cast<int>(size.x), static_cast<int>(size.y), _nodes.get(), static_cast<int>(size.x));
    stbrp_setup_allow_out_of_mem(_context.get(), 0);
}

std::optional<vec4u> rect_packer::pack(const vec2u& size) {
    stbrp_rect rect;
    rect.id = 0;
    rect.w = static_cast<stbrp_coord>(size.x);
    rect.h = static_cast<stbrp_coord>(size.y);
    rect.was_packed = 0;

    if (stbrp_pack_rects(_context.get(), &rect, 1)) {
        return vec4u{
            static_cast<unsigned int>(rect.x),
            static_cast<unsigned int>(rect.y),
            size.x,
            size.y
        };
    }
    return std::nullopt;
}
