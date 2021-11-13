#pragma once 

#include "system.hpp"
#include "viewport.hpp"

#include <memory>

namespace rb {
	class renderer : public rb::system {
		struct cache {
			mat4f world;
		};

	public:
		void initialize(registry& registry) override;

		void update(registry& registry, float elapsed_time) override;

		void draw(registry& registry) override;

	private:
		entity _find_directional_light_with_shadows(registry& registry) const;

		void _on_transform_constructed(registry& registry, entity entity);

	private:
		std::shared_ptr<viewport> _viewport;
	};
}
