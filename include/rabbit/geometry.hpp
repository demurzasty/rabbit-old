#pragma once 

#include "mesh.hpp"
#include "material.hpp"
#include "assets.hpp"

#include <memory>

namespace rb {
	struct geometry {
		std::shared_ptr<mesh> mesh;
		std::shared_ptr<material> material;

		template<typename Visitor>
		static void visit(Visitor& visitor, geometry& geometry) {
			visitor("mesh", assets::get_uuid(geometry.mesh));
			visitor("material", assets::get_uuid(geometry.material));
		}
	};
}
