#pragma once 

#include "color.hpp"
#include "../core/span.hpp"
#include "../math/vec2.hpp"
#include "../math/vec3.hpp"
#include "../math/vec4.hpp"

#include <vector>
#include <cstdint>

namespace rb {
    struct mesh_flags {
        static constexpr std::uint32_t normals_bit{ 1 << 16 };
        static constexpr std::uint32_t tangets_bit{ 1 << 17 };
        static constexpr std::uint32_t texcoords0_bit{ 1 << 18 };
        static constexpr std::uint32_t texcoords1_bit{ 1 << 19 };
        static constexpr std::uint32_t colors_bit{ 1 << 20 };
    };

    struct mesh_desc {
        span<const vec3f> positions;
        span<const vec3f> normals;
        span<const vec4f> tangents;
        span<const vec2f> texcoords0;
        span<const vec2f> texcoords1;
        span<const color> colors;
        span<const std::uint32_t> indices;
    };

    class mesh {
    public:
        virtual ~mesh() = default;

        span<const vec3f> positions() const noexcept;

        span<const vec3f> normals() const noexcept;

        span<const vec4f> tangents() const noexcept;

        span<const vec2f> texcoords0() const noexcept;

        span<const vec2f> texcoords1() const noexcept;

        span<const color> colors() const noexcept;

        span<const std::uint32_t> indices() const noexcept;

    protected:
        explicit mesh(const mesh_desc& desc);

    private:
        const std::vector<vec3f> _positions;
        const std::vector<vec3f> _normals;
        const std::vector<vec4f> _tangents;
        const std::vector<vec2f> _texcoords0;
        const std::vector<vec2f> _texcoords1;
        const std::vector<color> _colors;
        const std::vector<std::uint32_t> _indices;
    };
}
