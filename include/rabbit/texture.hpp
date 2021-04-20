#pragma once 

#include "config.hpp"
#include "vec2.hpp"

#include <cstddef>

namespace rb {
    enum class texture_format {
        r8,
        rg8,
        rgba8,
        d24s8
    };

    struct texture_desc {
        const void* data{ nullptr };
        vec2u size{ 0, 0 };
        texture_format format{ texture_format::rgba8 };
    };

    class texture {
    public:
        texture(const texture_desc& desc);

        virtual ~texture() = default;
    
        RB_NODISCARD const vec2u& size() const RB_NOEXCEPT;

        RB_NODISCARD vec2f texel() const RB_NOEXCEPT;

        RB_NODISCARD texture_format format() const RB_NOEXCEPT;

        RB_NODISCARD std::size_t bytes_per_pixel() const RB_NOEXCEPT;
    
    private:
        vec2u _size;
        texture_format _format;
    };
}
