#pragma once 

#include "system.hpp"
#include "viewport.hpp"

#include <memory>

namespace rb {
	class renderer : public rb::system {
	public:
		void initialize(registry& registry) override;

		void update(registry& registry, float elapsed_time) override;

		void draw(registry& registry) override;

	private:
		entity _find_directional_light_with_shadows(registry& registry) const;

		void _on_geometry_construct(registry& registry, entity entity);

	private:
		std::shared_ptr<viewport> _viewport;
	};
}
