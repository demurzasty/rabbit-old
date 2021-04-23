#pragma once

#include "../core/config.hpp"
#include "../math/vec2.hpp"
#include "../math/vec3.hpp"
#include "../math/vec4.hpp"

#include <cstdint>
#include <cstddef>
#include <vector>

namespace rb {
    enum class vertex_format_type : std::uint8_t {
        integer,
        floating_point
    };

    enum class vertex_attribute : std::uint8_t {
        position,
        texcoord,
        normal,
        blend_weight,
        blend_indices
    };

    struct vertex_format {
        struct hasher {
            RB_NODISCARD std::size_t operator()(const vertex_format& format) const RB_NOEXCEPT {
                return *reinterpret_cast<const std::uint32_t*>(&format);
            }
        };

        RB_NODISCARD static inline constexpr vertex_format vec2f() RB_NOEXCEPT {
            return { vertex_format_type::floating_point, 2, sizeof(rb::vec2f), false };
        }

        RB_NODISCARD static inline constexpr vertex_format vec3f() RB_NOEXCEPT {
            return { vertex_format_type::floating_point, 3, sizeof(rb::vec3f), false };
        }

        RB_NODISCARD static inline constexpr vertex_format vec4f() RB_NOEXCEPT {
            return { vertex_format_type::floating_point, 4, sizeof(rb::vec4f), false };
        }

        RB_NODISCARD static inline constexpr vertex_format vec2i() RB_NOEXCEPT {
            return { vertex_format_type::integer, 2, sizeof(rb::vec2i), false };
        }

        RB_NODISCARD static inline constexpr vertex_format vec3i() RB_NOEXCEPT {
            return { vertex_format_type::integer, 3, sizeof(rb::vec3i), false };
        }

        RB_NODISCARD static inline constexpr vertex_format vec4i() RB_NOEXCEPT {
            return { vertex_format_type::integer, 4, sizeof(rb::vec4i), false };
        }

        vertex_format_type type{ vertex_format_type::floating_point };
        std::uint8_t components{ 0 };
        std::uint8_t size{ 0 };
        bool normalized{ false };
    };

    struct vertex_element {
        vertex_attribute attribute{ vertex_attribute::position };
        vertex_format format;
    };

    class vertex_desc {
    public:
        using value_type = vertex_element;
        using allocator_type = std::vector<vertex_element>::allocator_type;
        using pointer = std::vector<vertex_element>::pointer;
        using const_pointer = std::vector<vertex_element>::const_pointer;
        using reference = vertex_element&;
        using const_reference = const vertex_element&;
        using size_type = std::vector<vertex_element>::size_type;
        using difference_type = std::vector<vertex_element>::difference_type;
        using difference_type = std::vector<vertex_element>::difference_type;

        using iterator = std::vector<vertex_element>::iterator;
        using const_iterator = std::vector<vertex_element>::const_iterator;

    public:
        vertex_desc() = default;

        vertex_desc(std::initializer_list<vertex_element> elements);

        vertex_desc(const vertex_desc&) = default;
        vertex_desc& operator=(const vertex_desc&) = default;

        vertex_desc(vertex_desc&&) = default;
        vertex_desc& operator=(vertex_desc&&) = default;

        RB_NODISCARD const vertex_element& operator[](std::size_t index) const;

        RB_NODISCARD iterator begin() RB_NOEXCEPT;
        RB_NODISCARD const_iterator begin() const RB_NOEXCEPT;

        RB_NODISCARD iterator end() RB_NOEXCEPT;
        RB_NODISCARD const_iterator end() const RB_NOEXCEPT;

        RB_NODISCARD bool empty() const RB_NOEXCEPT;
        RB_NODISCARD std::size_t size() const RB_NOEXCEPT;

        RB_NODISCARD std::size_t stride() const RB_NOEXCEPT;
        RB_NODISCARD std::size_t offset(std::size_t vertex_index) const RB_NOEXCEPT;

    private:
        std::vector<vertex_element> _elements;
    };
}
