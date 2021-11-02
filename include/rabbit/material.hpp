#pragma once 

#include "vec3.hpp"
#include "texture.hpp"
#include "json.hpp"
#include "bstream.hpp"
#include "fnv1a.hpp"

#include <memory>
#include <string>

namespace rb {
	struct material_desc {
		vec3f base_color{ 1.0f, 1.0f, 1.0f }; // TODO: Use color structure instead.
		float roughness{ 0.8f };
		float metallic{ 0.0f };
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

		static std::shared_ptr<material> load(bstream& stream);

		static void import(const std::string& input, const std::string& output, const json& metadata);

		virtual ~material() = default;

		const vec3f& base_color() const;

		float roughness() const;

		float metallic() const;

		const std::shared_ptr<texture>& albedo_map() const;

		const std::shared_ptr<texture>& normal_map() const;

		const std::shared_ptr<texture>& roughness_map() const;

		const std::shared_ptr<texture>& metallic_map() const;

		const std::shared_ptr<texture>& emissive_map() const;

		const std::shared_ptr<texture>& ambient_map() const;

	protected:
		material(const material_desc& desc);

	private:
		const vec3f _base_color;
		const float _roughness;
		const float _metallic;
		const std::shared_ptr<texture> _albedo_map;
		const std::shared_ptr<texture> _normal_map;
		const std::shared_ptr<texture> _roughness_map;
		const std::shared_ptr<texture> _metallic_map;
		const std::shared_ptr<texture> _emissive_map;
		const std::shared_ptr<texture> _ambient_map;
	};
}
