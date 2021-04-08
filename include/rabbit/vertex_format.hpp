#pragma once 

#include "vec2.hpp"
#include "vec3.hpp"
#include "vec4.hpp"

#include <cstdint>
#include <cstddef>

namespace rb {
    enum class vertex_format_type : std::uint8_t {
        integer,
        floating_point
    };

    struct vertex_format {
        static inline constexpr vertex_format vec2f() {
            return { vertex_format_type::floating_point, 2, sizeof(rb::vec2f), false };
        }

        static inline constexpr vertex_format vec3f() {
            return { vertex_format_type::floating_point, 3, sizeof(rb::vec3f), false };
        }

        static inline constexpr vertex_format vec4f() {
            return { vertex_format_type::floating_point, 4, sizeof(rb::vec4f), false };
        }

        static inline constexpr vertex_format vec2i() {
            return { vertex_format_type::integer, 2, sizeof(rb::vec2i), false };
        }

        static inline constexpr vertex_format vec3i() {
            return { vertex_format_type::integer, 3, sizeof(rb::vec3i), false };
        }

        static inline constexpr vertex_format vec4i() {
            return { vertex_format_type::integer, 4, sizeof(rb::vec4i), false };
        }

        vertex_format_type type{ vertex_format_type::floating_point };
        std::uint8_t components{ 0 };
        std::uint8_t size{ 0 };
        bool normalize{ false };
    };

    // enum class vertex_format : std::uint8_t {
    //     vec2f,
    //     vec3f,
    //     vec4f,
    //     vec2i,
    //     vec3i,
    //     vec4i
    // };

    // [[nodiscard]] std::size_t vertex_format_size(const vertex_format format);
}
