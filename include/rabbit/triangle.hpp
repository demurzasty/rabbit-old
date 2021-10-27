#pragma once 

#include "vec3.hpp"
#include "shape3.hpp"

namespace rb {
	template<typename T>
	struct triangle {
        constexpr const T& operator[](std::size_t index) const {
            return vertices[index];
        }

        constexpr T& operator[](std::size_t index) {
            return vertices[index];
        }

		vec3<T> vertices[3];
	};


    template<typename T>
    std::optional<intersection3<T>> intersect(const ray3<T>& ray, const triangle<T>& triangle) {
        const auto e1 = triangle[1] - triangle[0];
        const auto e2 = triangle[2] - triangle[0];

        const auto v1 = cross(ray.direction, e2);

        const auto d = dot(e1, v1);
        if (std::abs(d) < std::numeric_limits<T>::epsilon()) {
            return std::nullopt;
        }

        const auto u = dot(ray.position - triangle[0], v1) / d;
        if (u < 0 || u > 1) {
            return std::nullopt;
        }

        const auto v2 = cross(ray.position - triangle[0], e1);

        const auto v = dot(ray.direction, v2) / d;
        if (v < 0 || v > 1) {
            return std::nullopt;
        }

        const auto distance = dot(e2, v2) / d;
        if (distance < 0) {
            return std::nullopt;
        }

        intersection3<T> intersection;
        intersection.position = ray.position + ray.direction * distance;
        intersection.normal = normalize(cross(e1, e2));
        intersection.distance = distance;
        return intersection;
    }

    using trianglef = triangle<float>;
}
