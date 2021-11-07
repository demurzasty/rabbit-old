#pragma once 

#include "color.hpp"

namespace rb {
	struct light {
		color color{ color::white() };
		float intensity{ 1.0f };

		template<typename Visitor>
		static void visit(Visitor& visitor, light& light) {
			visitor("color", light.color);
			visitor("intensity", light.intensity);
		}
	};

	struct directional_light {
		bool shadow_enabled{ false };

		template<typename Visitor>
		static void visit(Visitor& visitor, directional_light& directional_light) {
			visitor("shadow_enabled", directional_light.shadow_enabled);
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
