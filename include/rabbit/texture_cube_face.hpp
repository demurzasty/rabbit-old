#pragma once

#include <cstdint>

namespace rb {
	enum class texture_cube_face : std::uint8_t {
		/**
		 * @brief Positive x-face of the texture cube.
		 */
		positive_x,

		/**
		 * @brief Negative x-face of the texture cube.
		 */
		negative_x,

		/**
		 * @brief Positive y-face of the texture cube.
		 */
		positive_y,

		/**
		 * @brief Negative y-face of the texture cube.
		 */
		negative_y,
		
		/**
		 * @brief Positive x-face of the texture cube.
		 */
		positive_z,

		/**
		 * @brief Negative z-face of the texture cube.
		 */
		negative_z
	};
}
