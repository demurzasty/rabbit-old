#pragma once 

#include <rabbit/engine/math/vec2.hpp>
#include <rabbit/engine/math/vec3.hpp>
#include <rabbit/engine/collision/ray3.hpp>
#include <rabbit/engine/collision/shape3.hpp>

#include <limits>

namespace rb {
	template<typename T>
	struct bbox {
		vec3<T> min;
		vec3<T> max;
	};

	template<typename T>
	std::optional<intersection3<T>> intersect(const ray3<T>& ray, const bbox<T>& box) {
		intersection3<T> intersection;
		intersection.distance = 0;

		auto tmax = std::numeric_limits<T>::max();

		for (auto i = 0u; i < 3u; ++i) {
			if (std::abs(ray.direction[i]) < std::numeric_limits<T>::epsilon()) {
				if (ray.position[i] < box.min[i] || ray.position[i] > box.max[i]) {
					return std::nullopt;
				}
			} else {
				const auto inverse = 1 / ray.direction[i];
				auto t1 = (box.min[i] - ray.position[i]) * inverse;
				auto t2 = (box.max[i] - ray.position[i]) * inverse;

				if (t1 > t2) {
					std::swap(t1, t2);
				}

				if (t1 > intersection.distance) {
					intersection.distance = t1;
				}

				if (t2 < tmax) {
					tmax = t2;
				}

				if (intersection.distance > tmax) {
					return std::nullopt;
				}
			}
		}

		intersection.position = ray.position + ray.direction * intersection.distance;
		intersection.normal = vec3<T>{ 0, 0, 0 }; // TODO: normal vector calculation
		return intersection;
	}

	using bboxi = bbox<int>;
	using bboxf = bbox<float>;
}
