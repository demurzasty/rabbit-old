#pragma once 

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
	};
}
