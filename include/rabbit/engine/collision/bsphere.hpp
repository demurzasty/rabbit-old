#pragma once 

#include <rabbit/engine/math/vec3.hpp>
#include <rabbit/engine/collision/ray3.hpp>
#include <rabbit/engine/collision/shape3.hpp>

namespace rb {
	template<typename T>
	struct bsphere {
		vec3<T> position;
		T radius;
	};

	template<typename T>
	std::optional<intersection3<T>> intersect(const ray3<T>& ray, const bsphere<T>& sphere) {
		const auto m = ray.position - sphere.position;
		
		const auto b = dot(m, ray.direction);
		const auto c = dot(m, m) - (sphere.radius * sphere.radius);
		if (c > 0 && b > 0) {
			return std::nullopt;
		}

		const auto d = b * b - c;
		if (d < 0) {
			return std::nullopt;
		}

		intersection3<T> intersection;
		intersection.distance = std::max(-b - std::sqrt(d), T{ 0 });
		intersection.position = ray.position + ray.direction * intersection.distance;
		return intersection;
	}

	using bspheref = bsphere<float>;
}
