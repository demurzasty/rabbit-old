#pragma once 

#include "../graphics/mesh.hpp"
#include "../graphics/material.hpp"

#include <memory>

namespace rb {
	struct geometry {
		std::shared_ptr<mesh> mesh;
		std::shared_ptr<material> material;

		template<typename Visitor>
		static void visit(Visitor& visitor, geometry& geometry) {
			visitor("mesh", geometry.mesh);
			visitor("material", geometry.material);
		}
	};

	struct cached_geometry {
		std::uint32_t lod_index{ 0 };
	};
}
