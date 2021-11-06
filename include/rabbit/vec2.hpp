#pragma once 

#include <cstddef>

namespace rb {
	template<typename T>
	struct vec2 {
		static constexpr vec2<T> zero() { return { 0, 0 }; }
		static constexpr vec2<T> one() { return { 1, 1 }; }
		static constexpr vec2<T> up() { return { 0, 1 }; }
		static constexpr vec2<T> x_axis() { return { 1, 0 }; }
		static constexpr vec2<T> y_axis() { return { 0, 1 }; }

		constexpr const T& operator[](std::size_t index) const {
			return (&x)[index];
		}

		constexpr T& operator[](std::size_t index) {
			return (&x)[index];
		}

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

	template<typename T>
	constexpr vec2<T> operator/(const vec2<T>& a, const T& b) {
		return { a.x / b, a.y / b };
	}

	using vec2i = vec2<int>;
	using vec2u = vec2<unsigned>;
	using vec2f = vec2<float>;
}
