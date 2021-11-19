#pragma once 

#include <rabbit/engine/graphics/vertex.hpp>
#include <rabbit/engine/core/span.hpp>
#include <rabbit/engine/core/json.hpp>
#include <rabbit/engine/collision/triangle.hpp>
#include <rabbit/engine/collision/bsphere.hpp>
#include <rabbit/engine/collision/bbox.hpp>
#include <rabbit/engine/core/bstream.hpp>
#include <rabbit/engine/core/fnv1a.hpp>

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <optional>

namespace rb {
	struct mesh_lod {
		std::uint32_t offset;
		std::uint32_t size;
	};

	struct mesh_desc {
		span<const vertex> vertices;
		span<const std::uint32_t> indices;
		span<const mesh_lod> lods;
		span<const trianglef> convex_hull;
		std::optional<bspheref> bsphere;
		std::optional<bboxf> bbox;
	};

	class mesh {
	public:
		static constexpr auto magic_number{ fnv1a("mesh") };

		static std::shared_ptr<mesh> load(ibstream& stream);

		static void import(ibstream& input, obstream& output, const json& metadata);

		static void save(obstream& stream, const span<const vertex>& vertices, const span<const std::uint32_t>& indices);

		static std::shared_ptr<mesh> make_box(const vec3f& extent, const vec2f& uv_scale);

		static std::shared_ptr<mesh> make_sphere(std::size_t stacks, std::size_t slices, float radius);

		virtual ~mesh() = default;

		span<const vertex> vertices() const;

		span<const std::uint32_t> indices() const;

		span<const mesh_lod> lods() const;

		span<const trianglef> convex_hull() const;

		const bspheref& bsphere() const;

		const bboxf& bbox() const;

	protected:
		mesh(const mesh_desc& desc);

	private:
		const std::vector<vertex> _vertices;
		const std::vector<std::uint32_t> _indices;
		const std::vector<mesh_lod> _lods;
		const std::vector<trianglef> _convex_hull;
		const bspheref _bsphere;
		const bboxf _bbox;
	};
}
