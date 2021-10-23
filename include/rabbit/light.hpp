#pragma once 

#include "color.hpp"

namespace rb {
	struct light {
		color color{ color::white() };
	};

	struct directional_light {
		int dummy{ 0 };
	};

	struct point_light {
		float radius{ 1.0f };
	};
}
