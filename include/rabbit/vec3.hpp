#pragma once 

namespace rb {
	template<typename T>
	struct vec3 {
		T x, y, z;
	};

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
	constexpr vec3<T> operator/(const vec3<T>& a, const vec3<T>& b) {
		return { a.x / b.x, a.y / b.y, a.z / b.z };
	}

	using vec3i = vec3<int>;
	using vec3u = vec3<unsigned>;
	using vec3f = vec3<float>;
}
