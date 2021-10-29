#pragma once 

#include "entity.hpp"

namespace rb {
	class system {
	public:
		virtual void initialize(registry& registry);

		virtual void update(registry& registry, float elapsed_time);

		virtual void draw(registry& registry);
	};
}