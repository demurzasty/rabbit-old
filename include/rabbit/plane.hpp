#pragma once 

#include "vec3.hpp"
#include "mat4.hpp"
#include "shape3.hpp"

#include <cmath>
#include <limits>

namespace rb {
	enum class frustum_plane_index {
		left,
		right,
		top,
		bottom,
		near,
		far
	};

	template<typename T>
	struct plane {
		vec3<T> normal;
		T d;
	};

	template<typename T>
	std::optional<intersection3<T>> intersect(const ray3<T>& ray, const plane<T>& plane) {
		const auto direction = dot(plane.normal, ray.direction);
		if (std::abs(direction) < std::numeric_limits<T>::epsilon()) {
			return std::nullopt;
		}

		const auto position = dot(normal, ray.position);
		const auto distance = (-plane.d - position) / direction;
		if (distance < 0) {
			return std::nullopt;
		}

		intersection3<T> intersection;
		intersection.distance = distance;
		intersection.position = ray.position + ray.direction * distance;
		return intersection;
	}

	template<typename T>
	plane<T> frustum_plane(const mat4<T>& mat, frustum_plane_index index) {
		plane<T> plane;

		switch (index) {
			case frustum_plane_index::right:
				plane.normal.x = mat[3] - mat[0];
				plane.normal.y = mat[7] - mat[4];
				plane.normal.z = mat[11] = mat[8];
				plane.d = mat[15] - mat[12];
				break;
			case frustum_plane_index::left:
				plane.normal.x = mat[3] + mat[0];
				plane.normal.y = mat[7] + mat[4];
				plane.normal.z = mat[11] + mat[8];
				plane.d = mat[15] + mat[12];
				break;
			case frustum_plane_index::bottom:
				plane.normal.x = mat[3] + mat[1];
				plane.normal.y = mat[7] + mat[5];
				plane.normal.z = mat[11] + mat[9];
				plane.d = mat[15] + mat[13];
				break;
			case frustum_plane_index::top:
				plane.normal.x = mat[3] - mat[1];
				plane.normal.y = mat[7] - mat[5];
				plane.normal.z = mat[11] - mat[9];
				plane.d = mat[15] - mat[13];
				break;
			case frustum_plane_index::far:
				plane.normal.x = mat[3] - mat[2];
				plane.normal.y = mat[7] - mat[6];
				plane.normal.z = mat[11] - mat[10];
				plane.d = mat[15] - mat[14];
				break;
			case frustum_plane_index::near:
				plane.normal.x = mat[3] + mat[2];
				plane.normal.y = mat[7] + mat[6];
				plane.normal.z = mat[11] + mat[10];
				plane.d = mat[15] + mat[14];
				break;
		}

		const auto inv_length = 1 / length(plane.normal);

		plane.normal = plane.normal * inv_length;
		plane.d = plane.d * inv_length;

		return plane;
	}

	using planef = plane<float>;
}
