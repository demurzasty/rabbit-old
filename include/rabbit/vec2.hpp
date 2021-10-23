#pragma once 

namespace rb {
	template<typename T>
	struct vec2 {
		T x, y;
	};

	template<typename T>
	constexpr vec2<T> operator-(const vec2<T>& a, const vec2<T>& b) {
		return { a.x - b.x, a.y - b.y };
	}

	template<typename T>
	constexpr vec2<T> operator+(const vec2<T>& a, const vec2<T>& b) {
		return { a.x + b.x, a.y + b.y };
	}

	template<typename T>
	constexpr vec2<T> operator*(const vec2<T>& a, const vec2<T>& b) {
		return { a.x * b.x, a.y * b.y };
	}

	template<typename T>
	constexpr vec2<T> operator/(const vec2<T>& a, const vec2<T>& b) {
		return { a.x / b.x, a.y / b.y };
	}

	using vec2i = vec2<int>;
	using vec2u = vec2<unsigned>;
	using vec2f = vec2<float>;
}
