#pragma once 

#include "vec2.hpp"
#include "vec3.hpp"
#include "vec4.hpp"
#include "mat4.hpp"

namespace rb {
	template<typename T>
	struct ray3 {
		vec3<T> position;
		vec3<T> direction;

		static ray3<T> pick_ray(const vec2<T>& point, const vec4<T>& viewport, const mat4<T>& inv_proj_view) {
			vec3<T> z_min{ point.x, point.y, 0 };
			vec3<T> z_far{ point.x, point.y, 1 };

			z_min = unproject(z_min, viewport, inv_proj_view);
			z_far = unproject(z_far, viewport, inv_proj_view);

			return { z_min, normalize(z_far - z_min) };
		}
	};

	using ray3f = ray3<float>;
}
