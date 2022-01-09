#pragma once 

#include "world.hpp"

namespace rb {
	class system {
	public:
		virtual void initialize();

		virtual void update(float elapsed_time);

		virtual void draw();
	};
}
