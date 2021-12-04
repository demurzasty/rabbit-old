#pragma once 

#include "../core/assets.hpp"
#include "color.hpp"
#include "texture.hpp"

#include <memory>
#include <cstdint>

namespace rb {
    struct material_flags {
        static constexpr std::uint32_t metallic_roughness_map_bit{ 1 << 0 };
        static constexpr std::uint32_t normal_map_bit{ 1 << 1 };
        static constexpr std::uint32_t occlusion_map_bit{ 1 << 2 };
        static constexpr std::uint32_t emissive_map_bit{ 1 << 3 };
        static constexpr std::uint32_t alpha_test_bit{ 1 << 4 };
        static constexpr std::uint32_t alpha_blend_bit{ 1 << 5 };
        static constexpr std::uint32_t double_sided_bit{ 1 << 6 };
        static constexpr std::uint32_t clearcoat_bit{ 1 << 7 };
        static constexpr std::uint32_t sheen_bit{ 1 << 8 };
        static constexpr std::uint32_t transmission_bit{ 1 << 9 };
        static constexpr std::uint32_t ior_bit{ 1 << 10 };
        static constexpr std::uint32_t volume_bit{ 1 << 11 };
        static constexpr std::uint32_t unlit_bit{ 1 << 12 };
    };

    enum class material_alpha_mode {
        opaque,
        alpha_test,
        alpha_blend
    };

    struct material_desc {
        color base_color{ 255, 255, 255, 255 };
        float metallic{ 1.0f };
        float roughness{ 1.0f };
        color emissive{ 0, 0, 0, 255 };
        material_alpha_mode alpha_mode{ material_alpha_mode::opaque };
        float alpha_cutoff{ 0.5f };
        bool double_sided{ false };
        bool clearcoat{ false };
        bool sheen{ false };
        bool transmission{ false };
        bool ior{ false };
        bool volume{ false };
        bool unlit{ false };
        asset<texture> albedo_map;
        asset<texture> metallic_roughness_map;
        asset<texture> normal_map;
        asset<texture> occlusion_map;
        asset<texture> emissive_map;
    };

    class material {
    public:
        virtual ~material() = default;

        color base_color() const noexcept;

        float metallic() const noexcept;

        float roughness() const noexcept;

        color emissive() const noexcept;

        material_alpha_mode alpha_mode() const noexcept;

        float alpha_cutoff() const noexcept;

        bool double_sided() const noexcept;

        bool clearcoat() const noexcept;

        bool sheen() const noexcept;

        bool transmission() const noexcept;

        bool ior() const noexcept;

        bool volume() const noexcept;

        bool unlit() const noexcept;

        const asset<texture>& albedo_map() const noexcept;

        const asset<texture>& metallic_roughness_map() const noexcept;

        const asset<texture>& normal_map() const noexcept;

        const asset<texture>& occlusion_map() const noexcept;

        const asset<texture>& emissive_map() const noexcept;

    protected:
        explicit material(const material_desc& desc);

    private:
        const color _base_color;
        const float _metallic;
        const float _roughness;
        const color _emissive;
        const material_alpha_mode _alpha_mode;
        const float _alpha_cutoff;
        const bool _double_sided;
        const bool _clearcoat;
        const bool _sheen;
        const bool _transmission;
        const bool _ior;
        const bool _volume;
        const bool _unlit;
        const asset<texture> _albedo_map;
        const asset<texture> _metallic_roughness_map;
        const asset<texture> _normal_map;
        const asset<texture> _occlusion_map;
        const asset<texture> _emissive_map;
    };
}
