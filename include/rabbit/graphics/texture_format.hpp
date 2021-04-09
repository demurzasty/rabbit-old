#pragma once

#include "../core/enum.hpp"

namespace rb {
	RB_ENUM(texture_format, std::uint8_t, "r8", "rg8", "rgba8", "d24s8")
	enum class texture_format : std::uint8_t {
		r8,
		rg8,
		rgba8,
		d24s8
	};
}
