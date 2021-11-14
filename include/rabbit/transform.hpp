#pragma once 

#include "entity.hpp"
#include "vec3.hpp"
#include "mat4.hpp"
#include "member.hpp"

namespace rb {
	/**
	 * @brief Due to performance reason you should commit changes using registry.patch(...) method.
	 */
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

	/**
	 * @brief Do not use this component directly.
	 */
	struct cached_transform {
		bool dirty{ true };
		mat4f world{ mat4f::identity() };
	};
}
