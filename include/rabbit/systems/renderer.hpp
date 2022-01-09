#pragma once 

#include "../core/system.hpp"
#include "../graphics/viewport.hpp"

#include <memory>

namespace rb {
	class renderer : public rb::system {
	public:
		void initialize() override;

		void update(float elapsed_time) override;

		void draw() override;

	private:
		entity _find_camera();
	};
}
