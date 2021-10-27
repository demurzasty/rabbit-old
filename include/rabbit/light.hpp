#pragma once 

#include "color.hpp"

namespace rb {
	struct light {
		color color{ color::white() };

		template<typename Visitor>
		static void visit(Visitor& visitor, light& light) {
			visitor("color", light.color);
		}
	};

	struct directional_light {
		int dummy{ 0 };

		template<typename Visitor>
		static void visit(Visitor& visitor, directional_light& directional_light) {
		}
	};

	struct point_light {
		float radius{ 1.0f };

		template<typename Visitor>
		static void visit(Visitor& visitor, point_light& point_light) {
			visitor("radius", point_light.radius);
		}
	};
}
