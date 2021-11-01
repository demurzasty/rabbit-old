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
		std::shared_ptr<viewport> _viewport;
	};
}
