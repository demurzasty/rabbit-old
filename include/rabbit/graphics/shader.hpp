#pragma once

#include "../core/span.hpp"
#include "vertex.hpp"
#include "../core/config.hpp"
#include "texture.hpp"

#include "../math/vec2.hpp"
#include "../math/vec3.hpp"
#include "../math/vec4.hpp"
#include "../math/mat4.hpp"

#include <cstdint>
#include <vector>
#include <string>
#include <variant>
#include <memory>
#include <optional>

namespace rb {
    struct shader_stage_flags {
        enum {
            vertex = (1 << 0),
            fragment = (1 << 1)
        };
    };

    enum class shader_parameter_type {
        floating_point,
        integer,
        vec2f,
        vec3f,
        vec4f,
        vec2i,
        vec3i,
        vec4i,
        mat4f,
        texture2d
    };

    /**
     * @brief Shader paramter type. Order of templates parameters must match shader_parameter_type enum.
     */
    using shader_parameter = std::variant<float, int, vec2f, vec3f, vec4f, vec2i, vec3i, vec4i, mat4f, std::shared_ptr<texture>>;

    /**
     * @brief
     */
    struct shader_parameter_desc {
        std::string name;
        shader_parameter_type type{ shader_parameter_type::floating_point };
        std::size_t offset{ 0 };
        std::size_t slot{ 0 };
        shader_parameter default_value;
    };

    enum class shader_binding_type {
        uniform_buffer,
        texture
    };

    /**
     * @brief Directly shader binding description used in shader.
     */
    struct shader_binding_desc {
        shader_binding_type binding_type{ shader_binding_type::uniform_buffer };
        std::uint32_t stage_flags{ 0 };
        std::size_t slot{ 0 };
        std::size_t array_size{ 1 };
        // std::string name; // ? Need in legacy API (GLES2)
    };

    struct shader_desc {
        vertex_desc vertex_desc;
        span<const std::uint32_t> vertex_bytecode;
        span<const std::uint32_t> fragment_bytecode;
        std::vector<shader_binding_desc> bindings;
        std::vector<shader_parameter_desc> parameters;
    };

    class shader {
    public:
        shader(const shader_desc& desc);

        virtual ~shader() = default;

        RB_NODISCARD const vertex_desc& vertex_desc() const RB_NOEXCEPT;

        RB_NODISCARD std::optional<shader_binding_desc> binding(std::size_t slot) const RB_NOEXCEPT;

        RB_NODISCARD const std::vector<shader_binding_desc>& bindings() const RB_NOEXCEPT;

        RB_NODISCARD const std::vector<shader_parameter_desc>& parameters() const RB_NOEXCEPT;

    private:
        rb::vertex_desc _vertex_desc;
        std::vector<shader_binding_desc> _bindings;
        std::vector<shader_parameter_desc> _parameters;
    };
}
