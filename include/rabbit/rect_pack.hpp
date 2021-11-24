#pragma once 

#include "vec2.hpp"
#include "vec4.hpp"

#include <memory>
#include <optional>

// Opaque types. Do not include stbrp directly.
typedef struct stbrp_context stbrp_context;
typedef struct stbrp_node    stbrp_node;

namespace rb {
    class rect_packer {
    public:
        rect_packer(const vec2u& size);

        rect_packer(const rect_packer&) = delete;
        rect_packer(rect_packer&&) = default;

        rect_packer& operator=(const rect_packer&) = delete;
        rect_packer& operator=(rect_packer&&) = default;

        std::optional<vec4u> pack(const vec2u& size);

    private:
        std::unique_ptr<stbrp_node[]> _nodes;
        std::unique_ptr<stbrp_context> _context;
    };
}