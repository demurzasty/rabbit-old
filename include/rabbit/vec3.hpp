#pragma once 

#include <cmath>

namespace rb {
	template<typename T>
	struct vec3 {
		static constexpr vec3<T> zero() { return { 0, 0, 0 }; }
		static constexpr vec3<T> one() { return { 1, 1, 1 }; }
		static constexpr vec3<T> up() { return { 0, 1, 0 }; }
		static constexpr vec3<T> x_axis() { return { 1, 0, 0 }; }
		static constexpr vec3<T> y_axis() { return { 0, 1, 0 }; }
		static constexpr vec3<T> z_axis() { return { 0, 0, 1 }; }

		constexpr const T& operator[](std::size_t index) const {
			return (&x)[index];
		}

		constexpr T& operator[](std::size_t index) {
			return (&x)[index];
		}

		T x, y, z;
	};

	template<typename T>
	constexpr vec3<T> operator-(const vec3<T>& a) {
		return { -a.x, -a.y, -a.z };
	}

	template<typename T>
	constexpr vec3<T> operator-(const vec3<T>& a, const vec3<T>& b) {
		return { a.x - b.x, a.y - b.y, a.z - b.z };
	}

	template<typename T>
	constexpr vec3<T> operator+(const vec3<T>& a, const vec3<T>& b) {
		return { a.x + b.x, a.y + b.y, a.z + b.z };
	}

	template<typename T>
	constexpr vec3<T> operator*(const vec3<T>& a, const vec3<T>& b) {
		return { a.x * b.x, a.y * b.y, a.z * b.z };
	}

	template<typename T>
	constexpr vec3<T> operator*(const vec3<T>& a, const T& b) {
		return { a.x * b, a.y * b, a.z * b };
	}

	template<typename T>
	constexpr vec3<T> operator/(const vec3<T>& a, const vec3<T>& b) {
		return { a.x / b.x, a.y / b.y, a.z / b.z };
	}

	template<typename T>
	constexpr vec3<T> operator/(const vec3<T>& a, const T& b) {
		return { a.x / b, a.y / b, a.z / b };
	}

	template<typename T>
	constexpr T dot(const vec3<T>& a, const vec3<T>& b) {
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	template<typename T>
	T length(const vec3<T>& v) {
		return std::sqrt(dot(v, v));
	}

	template<typename T>
	vec3<T> normalize(const vec3<T>& v) {
		return v / length(v);
	}

	template<typename T>
	vec3<T> cross(const vec3<T>& a, const vec3<T>& b) {
		return {
			(a.y * b.z) - (a.z * b.y),
			(a.z * b.x) - (a.x * b.z),
			(a.x * b.y) - (a.y * b.x)
		};
	}

	using vec3i = vec3<int>;
	using vec3u = vec3<unsigned>;
	using vec3f = vec3<float>;
}
