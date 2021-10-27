#pragma once 

#include "environment.hpp"
#include "assets.hpp"

#include <memory>

namespace rb {
	enum class camera_type {
		perspective,
		orthogonal
	};

	struct camera {
		camera_type type{ camera_type::perspective };
		float size{ 11.25 };
		float field_of_view{ 45.0f };
		float z_near{ 0.1f };
		float z_far{ 100.0f };
		std::shared_ptr<environment> environment;

		template<typename Visitor>
		static void visit(Visitor& visitor, camera& camera) {
			visitor("type", camera.type);
			visitor("size", camera.size);
			visitor("field_of_view", camera.field_of_view);
			visitor("z_near", camera.z_near);
			visitor("z_far", camera.z_far);
			visitor("environment", assets::get_uuid(camera.environment));
		}
	};
}
