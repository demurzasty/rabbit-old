#pragma once 

namespace rb {
	template<typename T>
	struct vec4 {
		static constexpr vec4<T> zero() { return { 0, 0, 0, 0 }; }
		static constexpr vec4<T> one() { return { 1, 1, 1, 0 }; }
		static constexpr vec4<T> x_axis() { return { 1, 0, 0, 0 }; }
		static constexpr vec4<T> y_axis() { return { 0, 1, 0, 0 }; }
		static constexpr vec4<T> z_axis() { return { 0, 0, 1, 0 }; }
		static constexpr vec4<T> w_axis() { return { 0, 0, 0, 1 }; }

		constexpr const T& operator[](std::size_t index) const {
			return (&x)[index];
		}

		constexpr T& operator[](std::size_t index) {
			return (&x)[index];
		}

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
