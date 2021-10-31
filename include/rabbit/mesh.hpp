#pragma once 

#include "vertex.hpp"
#include "span.hpp"
#include "json.hpp"
#include "triangle.hpp"
#include "bsphere.hpp"
#include "bstream.hpp"

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <optional>

namespace rb {
	struct mesh_desc {
		span<const vertex> vertices;
		span<const std::uint32_t> indices;
		span<const trianglef> convex_hull;
		std::optional<bspheref> bsphere;
	};

	class mesh {
	public:
		static std::shared_ptr<mesh> load(bstream& stream);

		static void import(const std::string& input, const std::string& output, const json& metadata);

		static std::shared_ptr<mesh> make_box(const vec3f& extent, const vec2f& uv_scale);

		static std::shared_ptr<mesh> mesh::make_sphere(std::size_t stacks, std::size_t slices, float radius);

		virtual ~mesh() = default;

		span<const vertex> vertices() const;

		span<const std::uint32_t> indices() const;

		span<const trianglef> convex_hull() const;

		const bspheref& bsphere() const;

	protected:
		mesh(const mesh_desc& desc);

	private:
		std::vector<vertex> _vertices;
		std::vector<std::uint32_t> _indices;
		std::vector<trianglef> _convex_hull;
		bspheref _bsphere;
	};
}
