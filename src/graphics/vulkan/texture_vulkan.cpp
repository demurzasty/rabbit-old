#include "texture_vulkan.hpp"

using namespace rb;


texture_vulkan::texture_vulkan(const texture_desc& desc)
    : texture(desc) {
}

texture_vulkan::~texture_vulkan() {
}

void texture_vulkan::update(const span<const std::uint8_t>& pixels, const vec4i& rect) {
}
