#pragma once 

#include "vertex.hpp"
#include "span.hpp"
#include "json.hpp"

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
		static std::shared_ptr<mesh> load(const std::string& filename, json& metadata);

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
