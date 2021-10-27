#pragma once 

#include "vertex.hpp"
#include "span.hpp"
#include "json.hpp"
#include "bstream.hpp"

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

namespace rb {
	struct mesh_desc {
		span<const vertex> vertices;
		span<const std::uint32_t> indices;
	};

	class mesh {
	public:
		static std::shared_ptr<mesh> load(bstream& stream);

		static void import(const std::string& input, const std::string& output, const json& metadata);

		static std::shared_ptr<mesh> make_box(const vec3f& extent, const vec2f& uv_scale);

		virtual ~mesh() = default;

		span<const vertex> vertices() const;

		span<const std::uint32_t> indices() const;

	protected:
		mesh(const mesh_desc& desc);

	private:
		std::vector<vertex> _vertices;
		std::vector<std::uint32_t> _indices;
	};
}
