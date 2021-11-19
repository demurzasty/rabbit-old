#pragma once 

#include <rabbit/engine/math/vec3.hpp>
#include <rabbit/engine/collision/ray3.hpp>

#include <optional>

namespace rb {
	template<typename T>
	struct intersection3 {
		vec3<T> position;
		vec3<T> normal;
		T distance;
	};

	using intersection3f = intersection3<float>;

	template<typename T>
	class shape3 {
	public:
		virtual ~shape3() = default;

		virtual std::optional<intersection3<T>> intersect(const ray3<T>& ray) const = 0;

	protected:
		shape3() = default;
	};
}
