#pragma once 

namespace rb {
	struct color {
		static constexpr color white() { return { 255, 255, 255, 255 }; }
		static constexpr color black() { return { 0, 0, 0, 255 }; }

		unsigned char r, g, b, a;
	};
}
