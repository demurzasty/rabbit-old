#pragma once 

#include "math.hpp"

#include <cmath> // std::cos, std::sin, std::pow

namespace rb {
	template<typename T>
	constexpr T ease_in_sine(const T& x) {
		return 1 - std::cos((x * pi<T>()) / 2);
	}

	template<typename T>
	constexpr T ease_out_sine(const T& x) {
		return std::sin((x * pi<T>()) / 2);
	}

	template<typename T>
	constexpr T ease_in_out_sine(const T& x) {
		return -(std::cos(x * pi<T>()) - 1) / 2;
	}

	template<typename T>
	constexpr T ease_in_quad(const T& x) {
		return x * x;
	}

	template<typename T>
	constexpr T ease_out_quad(const T& x) {
		return 1 - (1 - x) * (1 - x);
	}

	template<typename T>
	constexpr T ease_in_out_quad(const T& x) {
		return x < 0.5 ? 2 * x * x : 1 - std::pow(-2 * x + 2, 2) / 2;
	}

	template<typename T>
	constexpr T ease_in_cubic(const T& x) {
		return x * x * x;
	}

	template<typename T>
	constexpr T ease_out_cubic(const T& x) {
		return 1 - std::pow(1 - x, 3);
	}

	template<typename T>
	constexpr T ease_in_out_cubic(const T& x) {
		return x < 0.5 ? 4 * x * x * x : 1 - std::pow(-2 * x + 2, 3) / 2;
	}

	template<typename T>
	constexpr T ease_in_quart(const T& x) {
		return x * x * x * x;
	}

	template<typename T>
	constexpr T ease_out_quart(const T& x) {
		return 1 - std::pow(1 - x, 4);
	}

	template<typename T>
	constexpr T ease_in_out_quart(const T& x) {
		return x < 0.5 ? 8 * x * x * x * x : 1 - std::pow(-2 * x + 2, 4) / 2;
	}

	template<typename T>
	constexpr T ease_in_quint(const T& x) {
		return x * x * x * x * x;
	}

	template<typename T>
	constexpr T ease_out_quint(const T& x) {
		return 1 - std::pow(1 - x, 5);
	}

	template<typename T>
	constexpr T ease_in_out_quint(const T& x) {
		return x < 0.5 ? 16 * x * x * x * x * x : 1 - std::pow(-2 * x + 2, 5) / 2;
	}

	template<typename T>
	constexpr T ease_in_expo(const T& x) {
		return x == 0 ? 0 : std::pow(2, 10 * x - 10);
	}

	template<typename T>
	constexpr T ease_out_expo(const T& x) {
		return x == 1 ? 1 : 1 - std::pow(2, -10 * x);
	}

	template<typename T>
	constexpr T ease_in_out_expo(const T& x) {
		return x == 0
			? 0
			: x == 1
			? 1
			: x < 0.5 ? std::pow(2, 20 * x - 10) / 2
			: (2 - std::pow(2, -20 * x + 10)) / 2;
	}

	template<typename T>
	constexpr T ease_in_circ(const T& x) {
		return 1 - std::sqrt(1 - std::pow(x, 2));
	}

	template<typename T>
	constexpr T ease_out_circ(const T& x) {
		return std::sqrt(1 - std::pow(x - 1, 2));
	}

	template<typename T>
	constexpr T ease_in_out_circ(const T& x) {
		return x < 0.5
			? (1 - std::sqrt(1 - std::pow(2 * x, 2))) / 2
			: (std::sqrt(1 - std::pow(-2 * x + 2, 2)) + 1) / 2;
	}

	template<typename T>
	constexpr T ease_in_back(const T& x) {
		constexpr T c1 = 1.70158;
		constexpr T c3 = c1 + 1;

		return c3 * x * x * x - c1 * x * x;
	}

	template<typename T>
	constexpr T ease_out_back(const T& x) {
		constexpr T c1 = static_cast<T>(1.70158);
		constexpr T c3 = c1 + 1;

		return 1 + c3 * std::pow(x - 1, 3) + c1 * std::pow(x - 1, 2);
	}

	template<typename T>
	constexpr T ease_in_out_back(const T& x) {
		constexpr T c1 = 1.70158;
		constexpr T c2 = c1 * 1.525;

		return x < 0.5
			? (std::pow(2 * x, 2) * ((c2 + 1) * 2 * x - c2)) / 2
			: (std::pow(2 * x - 2, 2) * ((c2 + 1) * (x * 2 - 2) + c2) + 2) / 2;
	}

	template<typename T>
	constexpr T ease_in_elastic(const T& x) {
		constexpr T c4 = (2 * pi<T>()) / 3;

		return x == 0
			? 0
			: x == 1
			? 1
			: -std::pow(2, 10 * x - 10) * std::sin((x * 10 - 10.75) * c4);
	}

	template<typename T>
	constexpr T ease_out_elastic(const T& x) {
		constexpr T c4 = (2 * pi<T>()) / 3;

		return x == 0
			? 0
			: x == 1
			? 1
			: std::pow(2, -10 * x) * std::sin((x * 10 - 0.75) * c4) + 1;
	}

	template<typename T>
	constexpr T ease_in_out_elastic(const T& x) {
		constexpr T c5 = (2 * pi<T>()) / 4.5;

		return x == 0
			? 0
			: x == 1
			? 1
			: x < 0.5
			? -(std::pow(2, 20 * x - 10) * std::sin((20 * x - 11.125) * c5)) / 2
			: (std::pow(2, -20 * x + 10) * std::sin((20 * x - 11.125) * c5)) / 2 + 1;
	}

	template<typename T>
	constexpr T ease_out_bounce(const T& x) {
		constexpr T n1 = 7.5625;
		constexpr T d1 = 2.75;

		if (x < 1 / d1) {
			return n1 * x * x;
		} else if (x < 2 / d1) {
			return n1 * (x -= 1.5 / d1) * x + 0.75;
		} else if (x < 2.5 / d1) {
			return n1 * (x -= 2.25 / d1) * x + 0.9375;
		} else {
			return n1 * (x -= 2.625 / d1) * x + 0.984375;
		}
	}

	template<typename T>
	constexpr T ease_in_bounce(const T& x) {
		return 1 - ease_out_bounce(1 - x);
	}

	template<typename T>
	constexpr T ease_in_out_bounce(const T& x) {
		return x < 0.5
			? (1 - ease_out_bounce(1 - 2 * x)) / 2
			: (1 + ease_out_bounce(2 * x - 1)) / 2;
	}
}
