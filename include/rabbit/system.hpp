#pragma once 

#include "entity.hpp"
#include "mat4.hpp"
#include "transform.hpp"

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
