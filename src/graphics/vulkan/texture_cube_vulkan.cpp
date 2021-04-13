#include "texture_cube_vulkan.hpp"

using namespace rb;

texture_cube_vulkan::texture_cube_vulkan(const texture_cube_desc& desc)
    : texture_cube(desc) {
}

texture_cube_vulkan::~texture_cube_vulkan() {
}

void texture_cube_vulkan::update(texture_cube_face face, const span<const std::uint8_t>& pixels, const vec4i& rect) {
}
