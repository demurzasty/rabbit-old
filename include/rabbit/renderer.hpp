#pragma once 

#include "system.hpp"

namespace rb {
	class renderer : public rb::system {
	public:
		void draw(registry& registry) override;
	};
}
