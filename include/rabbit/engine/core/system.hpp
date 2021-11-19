#pragma once 

#include <rabbit/engine/core/entity.hpp>
#include <rabbit/engine/math/mat4.hpp>

// TODO: Remove, engine module should not depend on runtime module
#include <rabbit/runtime/components/transform.hpp>

namespace rb {
	class system {
	public:
		virtual void initialize(registry& registry);

		virtual void update(registry& registry, float elapsed_time);

		virtual void draw(registry& registry);

	protected:
		entity find_by_name(registry& registry, const std::string& name);

		const mat4f& get_world(registry& registry, entity entity, transform& transform);
	};
}
