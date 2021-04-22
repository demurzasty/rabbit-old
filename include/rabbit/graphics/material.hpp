#pragma once

#include "texture.hpp"
#include "../math/vec3.hpp"

#include <memory>

namespace rb {
    struct material_desc {
        vec3f base_color{ 1.0f, 1.0f, 1.0f };
        float roughness{ 0.8f };
        float metallic{ 0.0f };
        std::shared_ptr<texture> albedo_map{ nullptr };
        std::shared_ptr<texture> normal_map{ nullptr };
        std::shared_ptr<texture> roughness_map{ nullptr };
        std::shared_ptr<texture> metallic_map{ nullptr };
        std::shared_ptr<texture> emissive_map{ nullptr };
    };

    class material {
    public:
        material(const material_desc& desc);

        ~material() = default;

        RB_NODISCARD const vec3f& base_color() const RB_NOEXCEPT;

        RB_NODISCARD float roughness() const RB_NOEXCEPT;

        RB_NODISCARD float metallic() const RB_NOEXCEPT;

        RB_NODISCARD const std::shared_ptr<texture>& albedo_map() const RB_NOEXCEPT;

        RB_NODISCARD const std::shared_ptr<texture>& normal_map() const RB_NOEXCEPT;

        RB_NODISCARD const std::shared_ptr<texture>& roughness_map() const RB_NOEXCEPT;

        RB_NODISCARD const std::shared_ptr<texture>& metallic_map() const RB_NOEXCEPT;

        RB_NODISCARD const std::shared_ptr<texture>& emissive_map() const RB_NOEXCEPT;

    private:
        vec3f _base_color;
        float _roughness;
        float _metallic;
        std::shared_ptr<texture> _albedo_map;
        std::shared_ptr<texture> _normal_map;
        std::shared_ptr<texture> _roughness_map;
        std::shared_ptr<texture> _metallic_map;
        std::shared_ptr<texture> _emissive_map;
    };
}
