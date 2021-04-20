#pragma once 

#include "vertex.hpp"
#include "span.hpp"

#include <cstdint>
#include <vector>

namespace rb {
    struct shader_stage_flags {
        enum {
            vertex = (1 << 0),
            fragment = (1 << 1)
        };
    };

    enum class material_binding_type {
        uniform_buffer,
        texture,
        sampler
    };

    struct material_binding_desc { 
        material_binding_type binding_type{ material_binding_type::uniform_buffer };
        std::uint32_t stage_flags{ 0 };
        std::uint32_t slot{ 0 };
        std::uint32_t array_size{ 1 };
        const char* name{ nullptr };
    };

    struct material_desc {
        vertex_desc vertex_desc;
        span<const std::uint32_t> vertex_bytecode;
        span<const std::uint32_t> fragment_bytecode;
        std::vector<material_binding_desc> bindings;
    };

    class material {
    public:
        material(const material_desc& desc);

        ~material() = default;

        RB_NODISCARD const vertex_desc& vertex_desc() const RB_NOEXCEPT;

        RB_NODISCARD const std::vector<material_binding_desc>& bindings() const RB_NOEXCEPT;
        
    private:
        rb::vertex_desc _vertex_desc;
        std::vector<material_binding_desc> _bindings;
    };
}
