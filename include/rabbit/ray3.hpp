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

		template<typename T>
		static ray3<T> pick_ray(const vec2<T>& point, const vec4<T>& viewport, const mat4<T>& inv_proj_view) {
			vec3<T> near{ point.x, point.y, 0 };
			vec3<T> far{ point.x, point.y, 1 };

			near = unproject(near, viewport, inv_proj_view);
			far = unproject(far, viewport, inv_proj_view);

			return { near, normalize(far - near) };
		}
	};

	using ray3f = ray3<float>;
}
