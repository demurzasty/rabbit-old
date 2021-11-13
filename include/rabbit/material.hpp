#pragma once 

#include "vec3.hpp"
#include "texture.hpp"
#include "json.hpp"
#include "bstream.hpp"
#include "fnv1a.hpp"

#include <memory>
#include <string>

namespace rb {
	struct material_flags {
		static constexpr std::uint32_t albedo_map_bit = (1 << 0);
		static constexpr std::uint32_t normal_map_bit = (1 << 1);
		static constexpr std::uint32_t roughness_map_bit = (1 << 2);
		static constexpr std::uint32_t metallic_map_bit = (1 << 3);
		static constexpr std::uint32_t emissive_map_bit = (1 << 4);
		static constexpr std::uint32_t ambient_map_bit = (1 << 5);
		static constexpr std::uint32_t all_maps_bits = albedo_map_bit | normal_map_bit |
			roughness_map_bit | metallic_map_bit | emissive_map_bit | ambient_map_bit;
		static constexpr std::uint32_t translucent_bit = (1 << 6);
		static constexpr std::uint32_t double_sided_bit = (1 << 7);
	};

	struct material_desc {
		vec3f base_color{ 1.0f, 1.0f, 1.0f }; // TODO: Use color structure instead.
		float roughness{ 0.8f };
		float metallic{ 0.0f };
		bool translucent{ false };
		bool double_sided{ false };
		std::shared_ptr<texture> albedo_map;
		std::shared_ptr<texture> normal_map;
		std::shared_ptr<texture> roughness_map;
		std::shared_ptr<texture> metallic_map;
		std::shared_ptr<texture> emissive_map;
		std::shared_ptr<texture> ambient_map;
	};

	class material {
	public:
		static constexpr auto magic_number{ fnv1a("material") };

		static std::shared_ptr<material> load(ibstream& stream);

		static void import(ibstream& input, obstream& output, const json& metadata);

		virtual ~material() = default;

		const vec3f& base_color() const;

		float roughness() const;

		float metallic() const;

		bool translucent() const;

		bool double_sided() const;

		const std::shared_ptr<texture>& albedo_map() const;

		const std::shared_ptr<texture>& normal_map() const;

		const std::shared_ptr<texture>& roughness_map() const;

		const std::shared_ptr<texture>& metallic_map() const;

		const std::shared_ptr<texture>& emissive_map() const;

		const std::shared_ptr<texture>& ambient_map() const;

		const std::uint32_t flags() const;

	protected:
		material(const material_desc& desc);

	private:
		const vec3f _base_color;
		const float _roughness;
		const float _metallic;
		const bool _translucent;
		const bool _double_sided;
		const std::shared_ptr<texture> _albedo_map;
		const std::shared_ptr<texture> _normal_map;
		const std::shared_ptr<texture> _roughness_map;
		const std::shared_ptr<texture> _metallic_map;
		const std::shared_ptr<texture> _emissive_map;
		const std::shared_ptr<texture> _ambient_map;
		const std::uint32_t _flags;
	};
}
