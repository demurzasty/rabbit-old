#pragma once 

#include <rabbit/engine/core/system.hpp>
#include <rabbit/engine/graphics/viewport.hpp>

#include <memory>

namespace rb {
	class renderer : public rb::system {
	public:
		void initialize(registry& registry) override;

		void update(registry& registry, float elapsed_time) override;

		void draw(registry& registry) override;

	private:
		entity _find_directional_light_with_shadows(registry& registry) const;

	private:
		std::shared_ptr<viewport> _viewport;
	};
}
