#pragma once 

namespace rb {
	template<typename T>
	struct vec4 {
		T x, y, z, w;
	};

	template<typename T>
	constexpr vec4<T> operator-(const vec4<T>& a, const vec4<T>& b) {
		return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
	}

	template<typename T>
	constexpr vec4<T> operator+(const vec4<T>& a, const vec4<T>& b) {
		return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
	}

	template<typename T>
	constexpr vec4<T> operator*(const vec4<T>& a, const vec4<T>& b) {
		return { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
	}

	template<typename T>
	constexpr vec4<T> operator/(const vec4<T>& a, const vec4<T>& b) {
		return { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
	}

	using vec4i = vec4<int>;
	using vec4u = vec4<unsigned>;
	using vec4f = vec4<float>;
}
