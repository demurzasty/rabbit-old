#include "texture_vk.hpp"

#include <map>

using namespace rb;

static std::map<texture_format, int> bytes_per_pixels = {
	{ texture_format::r8, 1 },
	{ texture_format::rg8, 2 },
	{ texture_format::rgba8, 4 },
	{ texture_format::d24s8, 4 }
};

static std::map<texture_format, bool> depth_formats = {
	{ texture_format::r8, false },
	{ texture_format::rg8, false },
	{ texture_format::rgba8, false },
	{ texture_format::d24s8, true }
};

texture_vk::texture_vk(const texture_desc& desc)
	: texture(desc) {
}

texture_vk::~texture_vk() {
}

void texture_vk::update(const span<const std::uint8_t>& pixels, const vec4i& rect) {
}
