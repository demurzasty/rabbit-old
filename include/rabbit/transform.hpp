#pragma once 

#include "entity.hpp"
#include "vec3.hpp"

namespace rb {
	struct transform {
		entity parent{ null };
		vec3f position{ 0.0f, 0.0f, 0.0f };
		vec3f rotation{ 0.0f, 0.0f, 0.0f };
		vec3f scaling{ 1.0f, 1.0f, 1.0f };

		template<typename Visitor>
		static void visit(Visitor& visitor, transform& transform) {
			visitor("position", transform.position);
			visitor("rotation", transform.rotation);
			visitor("scaling", transform.scaling);
		}
	};
}
